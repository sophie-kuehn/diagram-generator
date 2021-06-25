#ifndef SDG_H
#define SDG_H

#include <string>
#include <vector>
#include <map>
#include <cairo.h>
#include <cairo-svg.h>

namespace SDG
{
    const auto DEFAULT_FONT_FAMILY = "Tahoma";
    const double DEFAULT_FONT_SIZE = 14;

    const auto DEFAULT_START_CAP = "";
    const auto DEFAULT_END_CAP = "arrowIn";
    const double DEFAULT_CAP_SIZE = 3;

    const double DEFAULT_ROOT_BOX_MARGIN = 10;
    const double DEFAULT_BOX_PADDING = 5;
    const double DEFAULT_BOX_CHILDREN_PADDING = 5;
    const double DEFAULT_BOX_LINK_PADDING = 5;

    void log(const std::string &msg, bool clear = false);

    struct Position
    {
        double x = 0;
        double y = 0;
    };

    struct Dimension
    {
        double width = 0;
        double height = 0;
    };

    struct Font
    {
        const char* family = DEFAULT_FONT_FAMILY;
        double size = DEFAULT_FONT_SIZE;
        cairo_font_slant_t slant = CAIRO_FONT_SLANT_NORMAL;
        cairo_font_weight_t weight = CAIRO_FONT_WEIGHT_NORMAL;
    };

    class Box;

    class BoxLink
    {
      public:
        Box* origin = nullptr;
        Box* target = nullptr;
        Box* lastStop = nullptr;
        Position lastExit;
        std::string startCap;
        std::string endCap;
        int interLane = 0;
        bool reached = false;
        bool skipLaneAssignment = false;

        BoxLink() = default;

        void reset();
    };

    class Box
    {
      public:
        std::string text;
        std::vector<Box*> children;
        std::vector<BoxLink*> links;
        std::vector<BoxLink*> processedLinks;
        Font font;
        double padding = DEFAULT_BOX_PADDING;
        double childrenPadding = DEFAULT_BOX_CHILDREN_PADDING;
        double linkPadding = DEFAULT_BOX_LINK_PADDING;
        double rootMargin = DEFAULT_ROOT_BOX_MARGIN;
        bool horizontalArrangement = false;
        bool rendered = false;
        Position positionInParent;
        Dimension renderedDimension;
        cairo_surface_t* surface = nullptr;

        Box() = default;

        void addLink(Box* target, std::string startCap, std::string endCap);
        void reset();
    };

    class BoxRenderer
    {
      public:
        BoxRenderer() = default;

        void render(Box* box, const std::string& fileName, int lod);

      private:
        void internalRender(Box* box, int lod);
        BoxLink* findMirrorLink(BoxLink* link, std::vector<BoxLink*> list);
        Dimension drawMultiLineText(cairo_t* cairo, std::string &text);
        void drawLineCap(cairo_t* cairo, BoxLink* link, Position position, bool isEnd);
    };
}

#endif
