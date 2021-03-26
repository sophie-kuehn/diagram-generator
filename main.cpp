#include <iostream>
#include <cmath>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <cairo.h>
#include <cairo-svg.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

void print(const std::string &msg)
{
    std::cout << msg.c_str() << std::endl;
}

void replaceInString(std::string& target, const std::string needle, const std::string replace)
{
    while(target.find(needle) != std::string::npos) {
        target.replace(target.find(needle), needle.size(), replace);
    }
}

std::vector<std::string> splitString(const std::string& s, char delim)
{
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string elem;
    while(std::getline(ss, elem, delim)) elems.push_back(elem);
    return elems;
}

struct Position {
    double x;
    double y;
};

struct Dimension {
    double width;
    double height;
};

class Box;

class BoxLink {
    public:
        // initial properties

        Box* origin = nullptr;
        Box* target = nullptr;

        // process properties

        Box* lastStop = nullptr;
        Position lastExit = {0,0};
        int interLane = 0;
        bool reached = false;
        bool skipLaneAssignment = false;

        BoxLink() = default;

        void reset()
        {
            this->interLane = 0;
            this->reached = false;
            this->lastExit = {0,0};
            this->lastStop = this->origin;
        }

};

class Box
{
    public:
        // initial properties

        std::string text;
        std::vector<Box*> children;
        std::vector<BoxLink*> links;
        double padding = 5;
        double linkPadding = 5;
        bool horizontalArrangement = false;

        Box() = default;

        void addLink(Box* target)
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

        void addChild(Box* child)
        {
            this->children.push_back(child);
        }

        // process properties

        cairo_surface_t* surface = nullptr;
        std::vector<BoxLink*> processedLinks;
        Position positionInParent = {0,0};
        Dimension renderedDimension = {0,0};

        void reset()
        {
            this->renderedDimension = {0,0};
            this->positionInParent = {0,0};
            this->processedLinks.clear();

            for (auto &link : this->links) {
                link->reset();
                this->processedLinks.push_back(link);
            }
        }
};

class BoxRenderer
{
    public:
        BoxRenderer() = default;
        double rootPadding = 20;

        void render(Box* box, const std::string& fileName)
        {
            this->internalRender(box);

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

        void internalRender(Box* box)
        {
            // init

            box->reset();
            box->surface = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, nullptr);
            auto cairo = cairo_create(box->surface);

            std::vector<Position> blockedSpots;
            std::vector<BoxLink*> childrenLinks;
            double drawPosX = box->padding;
            double drawPosY = box->padding;
            double maxChildrenWidth = 0;
            double maxChildrenHeight = 0;
            int interLaneCount = 0;

            // trigger internal render for children and transfer links

            for (const auto& child : box->children) {
                this->internalRender(child);
                for (const auto &link : child->processedLinks) {
                    childrenLinks.push_back(link);
                }
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
                cairo_move_to(cairo, drawPosX, drawPosY);
                cairo_select_font_face(cairo, "Tahoma", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
                cairo_set_font_size(cairo, 14);
                Dimension textDimensions = this->drawMultiLineText(cairo, box->text);
                maxChildrenWidth = textDimensions.width;
                maxChildrenHeight = textDimensions.height;
                drawPosY = drawPosY + textDimensions.height + box->padding;
            }

            // draw children

            drawPosX = drawPosX + (interLaneCount * box->linkPadding);

            for (const auto& child : box->children) {
                // render child box
                cairo_set_source_surface(cairo, child->surface, drawPosX, drawPosY);
                cairo_paint(cairo);

                // save positional values
                child->positionInParent = {drawPosX, drawPosY};
                if (child->renderedDimension.width > maxChildrenWidth) maxChildrenWidth = child->renderedDimension.width;
                if (child->renderedDimension.height > maxChildrenHeight) maxChildrenHeight = child->renderedDimension.height;

                // move draw pos for next child
                if (box->horizontalArrangement) {
                    drawPosX = drawPosX + child->renderedDimension.width + box->padding;
                } else {
                    drawPosY = drawPosY + child->renderedDimension.height + box->padding;
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
                box->renderedDimension.width = drawPosX;
                box->renderedDimension.height = maxChildrenHeight + (box->padding * 2);
            } else {
                box->renderedDimension.width = maxChildrenWidth + (box->padding * 2) + (interLaneCount * box->linkPadding);
                box->renderedDimension.height = drawPosY;
            }

            // draw border

            cairo_set_source_rgb(cairo, 0, 0, 0);
            cairo_set_line_width(cairo, 2);
            cairo_rectangle(cairo, 0, 0, box->renderedDimension.width, box->renderedDimension.height);
            cairo_stroke(cairo);
        }

        BoxLink* findMirrorLink(BoxLink* link, std::vector<BoxLink*> list)
        {
            for (const auto& potentialTargetLink : list) {
                if (potentialTargetLink->target == link->origin
                    && potentialTargetLink->origin == link->target
                        ) return potentialTargetLink;
            }
            return nullptr;
        }

        Dimension drawMultiLineText(cairo_t* cairo, std::string &text)
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
};

struct XmlReaderLink
{
    std::string origin;
    std::string target;
};

class XmlReader
{
    private:
        std::map<std::string, Box*> boxIdMap;
        std::vector<XmlReaderLink> links;

        int fallbackId = 0;
        std::string getFallbackId()
        {
            std::string id = std::to_string(this->fallbackId);
            while (this->boxIdMap.count(id) > 0) {
                this->fallbackId++;
                id = std::to_string(this->fallbackId);
            }
            return id;
        }

    public:
        XmlReader() = default;

        Box* read(const std::string& filePath)
        {
            this->boxIdMap.clear();

            xmlDoc* doc = xmlReadFile(filePath.c_str(), nullptr, 0);
            if (doc == nullptr) {
                throw std::runtime_error("Could not parse config file from " + filePath);
            }

            auto box = this->processNodes(xmlDocGetRootElement(doc), "")[0];

            for (const auto& link : this->links) {
                if (this->boxIdMap.count(link.origin) > 0 && this->boxIdMap.count(link.target) > 0) {
                    this->boxIdMap.at(link.origin)->addLink(this->boxIdMap.at(link.target));
                }
            }

            xmlFreeDoc(doc);
            xmlCleanupParser();

            return box;
        }

        std::vector<Box*> processNodes(xmlNode* nodes, const std::string& parentId)
        {
            std::vector<Box*> boxes;
            xmlNode* node = nullptr;

            for (node = nodes; node; node = node->next) {
                if (node->type != XML_ELEMENT_NODE) continue;
                std::string name = (const char*)node->name;

                if (name == "link" && !parentId.empty()) {
                    xmlChar* target = xmlGetProp(node, (const xmlChar*)"target");
                    if (target != nullptr) {
                        this->links.push_back({parentId, (const char*)target});
                    }

                } else if (name == "box") {
                    auto box = new Box();

                    xmlChar* text = xmlGetProp(node, (const xmlChar*)"text");
                    if (text != nullptr) {
                        box->text = (const char*)text;
                        replaceInString(box->text, "\\n", "\n");
                    }

                    std::string sId;
                    xmlChar* id = xmlGetProp(node, (const xmlChar*)"id");
                    if (id != nullptr) {
                        sId = (const char*)id;
                    } else {
                        sId = this->getFallbackId();
                    }

                    this->boxIdMap[sId] = box;
                    box->children = this->processNodes(node->children, sId);
                    boxes.push_back(box);
                }
            }


            return boxes;
        }
};

int main()
{
    auto reader = new XmlReader();
    auto renderer = new BoxRenderer();
    renderer->render(reader->read("../example/example.xml"), "../example/example.svg");

    return 0;
}
