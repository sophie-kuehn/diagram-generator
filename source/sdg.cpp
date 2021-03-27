#include <iostream>
#include <sstream>

#include "sdg.h"

namespace SDG
{

    void print(const std::string &msg)
    {
        std::cout << msg.c_str() << std::endl;
    }

    std::vector<std::string> splitString(const std::string& s, char delim)
    {
        std::vector<std::string> elems;
        std::stringstream ss(s);
        std::string elem;
        while(std::getline(ss, elem, delim)) elems.push_back(elem);
        return elems;
    }

    void BoxLink::reset()
    {
        this->interLane = 0;
        this->reached = false;
        this->lastExit = {0, 0};
        this->lastStop = this->origin;
    }

    void Box::addLink(SDG::Box* target)
    {
        auto link = new BoxLink();
        link->target = target;
        link->origin = this;
        this->links.push_back(link);

        auto mirror = new BoxLink();
        mirror->target = this;
        mirror->origin = target;
        target->links.push_back(mirror);
    }

    void Box::reset()
    {
        this->rendered = false;
        this->renderedDimension = {0, 0};
        this->positionInParent = {0, 0};
        this->processedLinks.clear();

        for (auto& link : this->links) {
            link->reset();
            this->processedLinks.push_back(link);
        }
    }

    void BoxRenderer::render(SDG::Box* box, const std::string& fileName, int lod)
    {
        this->internalRender(box, lod);

        auto surface = cairo_svg_surface_create(fileName.c_str(),
            box->renderedDimension.width + (this->rootPadding * 2),
            box->renderedDimension.height + (this->rootPadding * 2)
        );

        auto cairo = cairo_create(surface);
        cairo_set_source_rgb(cairo, 1, 1, 1);
        cairo_paint(cairo);
        cairo_set_source_surface(cairo, box->surface, this->rootPadding, this->rootPadding);
        cairo_paint(cairo);
        cairo_surface_flush(surface);
        cairo_surface_finish(surface);
    }

    void BoxRenderer::internalRender(SDG::Box* box, int lod)
    {
        // init

        box->reset();
        box->surface = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, nullptr);
        auto cairo = cairo_create(box->surface);

        std::vector<Position> blockedSpots;
        std::vector<BoxLink*> childrenLinks;
        Position drawPos = {box->padding,box->padding};
        Dimension maxChildrenSize = {0,0};
        int interLaneCount = 0;

        lod--;

        // trigger internal render for children and transfer links

        for (const auto& child : box->children) {
            this->internalRender(child, lod);
            for (const auto& link : child->processedLinks) {
                childrenLinks.push_back(link);
            }
        }

        // just pass external links though if lod is below zero

        if (lod < 0) {
            for (const auto& link : childrenLinks) {
                BoxLink* targetLink = this->findMirrorLink(link, childrenLinks);
                if (targetLink != nullptr) continue;
                link->lastStop = box;
                link->lastExit = {0, 0};
                box->processedLinks.push_back(link);
            }
            return;
        }

        // find number of internal lanes and add assign lanes

        for (const auto& link : childrenLinks) {
            if (link->skipLaneAssignment) continue;
            BoxLink* targetLink = this->findMirrorLink(link, childrenLinks);
            if (targetLink != nullptr) {
                targetLink->skipLaneAssignment = true;
                link->interLane = interLaneCount + 1;
                interLaneCount++;
            }
        }

        // draw text

        if (!box->text.empty()) {
            cairo_set_source_rgb(cairo, 0, 0, 0);
            cairo_move_to(cairo, drawPos.x, drawPos.y);
            cairo_select_font_face(cairo, "Tahoma", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
            cairo_set_font_size(cairo, 14);
            Dimension textDimensions = this->drawMultiLineText(cairo, box->text);
            maxChildrenSize.width = textDimensions.width;
            maxChildrenSize.height = textDimensions.height;
            drawPos.y = drawPos.y + textDimensions.height + box->padding;
        }

        // draw children

        drawPos.x = drawPos.x + (interLaneCount * box->linkPadding);

        for (const auto& child : box->children) {
            if (!child->rendered) {
                continue;
            }

            // render child box
            cairo_set_source_surface(cairo, child->surface, drawPos.x, drawPos.y);
            cairo_paint(cairo);

            // save positional values
            child->positionInParent = {drawPos.x, drawPos.y};

            if (child->renderedDimension.width > maxChildrenSize.width)
                maxChildrenSize.width = child->renderedDimension.width;

            if (child->renderedDimension.height > maxChildrenSize.height)
                maxChildrenSize.height = child->renderedDimension.height;

            // move draw pos for next child
            if (box->horizontalArrangement) {
                drawPos.x = drawPos.x + child->renderedDimension.width + box->padding;
            } else {
                drawPos.y = drawPos.y + child->renderedDimension.height + box->padding;
            }

            // translate last exit to origin box in links
            for (const auto& link : child->processedLinks) {
                link->lastExit.x = link->lastExit.x + link->lastStop->positionInParent.x;
                link->lastExit.y = link->lastExit.y + link->lastStop->positionInParent.y;
            }
        }

        // draw children links

        for (const auto& link : childrenLinks) {
            link->skipLaneAssignment = false; // just to reset
            if (link->reached) continue;

            // separate lines
            /*
            if (link->origin == link->lastStop) {
                bool foundSpot = false;
                while (!foundSpot) {
                    bool moved = false;
                    for (auto &blockedSpot : blockedSpots) {
                        if (link->lastExit.y == blockedSpot.y) {
                            link->lastExit.y = link->lastExit.y + box->linePadding;
                            moved = true;
                            break;
                        }
                    }
                    if (!moved) foundSpot = true;
                }
            }

            blockedSpots.push_back({link->lastExit.x, link->lastExit.y});*/

            // render preparation
            cairo_set_source_rgb(cairo, 0, 0, 1);
            cairo_set_line_width(cairo, 1);
            cairo_move_to(cairo, link->lastExit.x, link->lastExit.y);

            // find target link if it exists
            BoxLink* targetLink = this->findMirrorLink(link, childrenLinks);

            if (targetLink != nullptr) {
                // internal link

                cairo_line_to(cairo, link->lastExit.x - (box->linkPadding * link->interLane), link->lastExit.y);
                cairo_line_to(cairo, targetLink->lastExit.x - (box->linkPadding * link->interLane), targetLink->lastExit.y);
                cairo_line_to(cairo, targetLink->lastExit.x, targetLink->lastExit.y);

                link->reached = true;
                targetLink->reached = true;

                print("CONNECTED | " + box->text + " | " + link->origin->text + " | " + link->target->text);

            } else {
                // external link

                cairo_line_to(cairo, 0, link->lastExit.y);
                link->lastStop = box;
                link->lastExit = {0, link->lastExit.y};
                box->processedLinks.push_back(link);

                print("PASSED THROUGH | " + box->text + " | " + link->origin->text + " | " + link->target->text);
            }

            cairo_stroke(cairo);
        }

        // calc inner width and height of box

        if (box->horizontalArrangement) {
            box->renderedDimension.width = drawPos.x;

            box->renderedDimension.height = maxChildrenSize.height
                + (box->padding * 2);
        } else {
            box->renderedDimension.width = maxChildrenSize.width
                + (box->padding * 2)
                + (interLaneCount * box->linkPadding);

            box->renderedDimension.height = drawPos.y;
        }

        // draw border

        cairo_set_source_rgb(cairo, 0, 0, 0);
        cairo_set_line_width(cairo, 2);
        cairo_rectangle(cairo, 0, 0, box->renderedDimension.width, box->renderedDimension.height);
        cairo_stroke(cairo);
        box->rendered = true;
    }

    Dimension BoxRenderer::drawMultiLineText(cairo_t* cairo, std::string& text)
    {
        std::vector<std::string> lines = splitString(text, '\n');
        Dimension extends = {};
        cairo_text_extents_t lineExtends = {};

        for (const auto& line : lines) {
            cairo_text_extents(cairo, line.c_str(), &lineExtends);

            cairo_rel_move_to(cairo, 0, lineExtends.height);
            cairo_text_path(cairo, line.c_str());
            cairo_rel_move_to(cairo, -lineExtends.x_advance, 0);

            extends.height = extends.height + lineExtends.height;
            if (lineExtends.x_advance > extends.width) extends.width = lineExtends.x_advance;
        }

        cairo_fill(cairo);
        cairo_stroke(cairo);

        return extends;
    }

    BoxLink* BoxRenderer::findMirrorLink(SDG::BoxLink* link, std::vector<SDG::BoxLink*> list)
    {
        for (const auto& potentialTargetLink : list) {
            if (potentialTargetLink->target == link->origin
                && potentialTargetLink->origin == link->target
                )
                return potentialTargetLink;
        }
        return nullptr;
    }

}
