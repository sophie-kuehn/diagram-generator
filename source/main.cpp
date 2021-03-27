#include "sdg.h"
#include "xmlLoader.h"

int main()
{
    auto loader = new SDG::XmlLoader();
    auto renderer = new SDG::BoxRenderer();
    renderer->render(loader->load("../example/example.xml"), "../example/example.svg", 9999);
    renderer->render(loader->load("../example/example.xml"), "../example/example_lod2.svg", 2);

    return 0;
}
