#ifndef SDG_H
#define SDG_H

#include <string>
#include <vector>
#include <map>
#include <cairo.h>
#include <cairo-svg.h>

namespace SDG
{

    struct Position
    {
        double x;
        double y;
    };

    struct Dimension
    {
        double width;
        double height;
    };

    class Box;

    class BoxLink
    {
      public:
        Box* origin = nullptr;
        Box* target = nullptr;
        Box* lastStop = nullptr;
        Position lastExit = {0,0};
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
        double padding = 5;
        double linkPadding = 5;
        bool horizontalArrangement = false;
        bool rendered = false;
        Position positionInParent = {0,0};
        Dimension renderedDimension = {0,0};
        cairo_surface_t* surface = nullptr;

        Box() = default;

        void addLink(Box* target);
        void reset();
    };

    class BoxRenderer
    {
      public:
        double rootPadding = 20;

        BoxRenderer() = default;

        void render(Box* box, const std::string& fileName, int lod);

      private:
        void internalRender(Box* box, int lod);
        BoxLink* findMirrorLink(BoxLink* link, std::vector<BoxLink*> list);
        Dimension drawMultiLineText(cairo_t* cairo, std::string &text);
    };

}

#endif