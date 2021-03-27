#include <iostream>

#include "sdg.h"
#include "xmlLoader.h"

int main(int argc, char* argv[])
{
    std::string inputFile;
    std::string outputFile = "output.svg";
    int lod = 9999;

    if (argc >= 2) inputFile = argv[1];
    if (argc >= 3) outputFile = argv[2];
    if (argc >= 4) lod = std::atoi(argv[3]);

    if (argc < 2 || inputFile.empty() || outputFile.empty()) {
        std::cout << "One or more arguments are missing. Usage:" << std::endl;
        std::cout << "./SophiesDiagramGenerator /path/to/input.xml /path/to/output.csv [optional integer LOD setting]" << std::endl;
        return 1;
    }

    if (lod <= 1) {
        std::cout << "Invalid LOD value: Has to be greater than or equal to 2." << std::endl;
        return 1;
    }

    auto loader = new SDG::XmlLoader();
    auto renderer = new SDG::BoxRenderer();
    renderer->render(loader->load(inputFile), outputFile, lod);

    return 0;
}
