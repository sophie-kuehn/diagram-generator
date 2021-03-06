#ifndef XML_LOADER_H
#define XML_LOADER_H

#include <string>
#include <vector>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "sdg.h"

namespace SDG
{
    struct XmlLoaderLink
    {
        std::string origin;
        std::string target;
        std::string startCap;
        std::string endCap;
    };

    class XmlLoader
    {
      private:
        std::map<std::string, Box*> boxIdMap;
        std::vector<XmlLoaderLink> links;
        int fallbackId = 0;

        std::string getFallbackId();
        std::string getPropValue(xmlNode* node, std::string name);

      public:
        XmlLoader() = default;

        Box* load(const std::string& filePath);
        std::vector<Box*> processNodes(xmlNode* nodes, const std::string& parentId);
    };
}

#endif
