#ifndef SDG_H
#define SDG_H

#include <string>
#include <vector>
#include <map>
#include <cairo.h>
#include <cairo-svg.h>

namespace SDG
{

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
        const char* family = "Tahoma";
        double size = 14;
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
        double padding = 5;
        double childrenPadding = 5;
        double linkPadding = 5;
        double rootMargin = 10;
        bool horizontalArrangement = false;
        bool rendered = false;
        Position positionInParent;
        Dimension renderedDimension;
        cairo_surface_t* surface = nullptr;

        Box() = default;

        void addLink(Box* target);
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
    };

}

#endif
