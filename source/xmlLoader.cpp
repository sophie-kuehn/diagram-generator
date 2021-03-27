#include <iostream>
#include "xmlLoader.h"

namespace SDG
{

    void replaceInString(std::string& target, const std::string& needle, const std::string& replace)
    {
        while(target.find(needle) != std::string::npos) {
            target.replace(target.find(needle), needle.size(), replace);
        }
    }

    std::string XmlLoader::getFallbackId()
    {
        std::string id = std::to_string(this->fallbackId);
        while (this->boxIdMap.count(id) > 0) {
            this->fallbackId++;
            id = std::to_string(this->fallbackId);
        }
        return id;
    }

    Box* XmlLoader::load(const std::string& filePath)
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

    std::vector<Box*> XmlLoader::processNodes(xmlNode* nodes, const std::string& parentId)
    {
        std::vector<Box*> boxes;
        xmlNode* node = nullptr;

        for (node = nodes; node; node = node->next) {
            if (node->type != XML_ELEMENT_NODE) continue;
            std::string name = (const char*)node->name;

            if (name == "link" && !parentId.empty()) {
                std::string linkTarget = this->getPropValue(node, "target");
                if (!linkTarget.empty()) this->links.push_back({parentId, linkTarget});

            } else if (name == "box") {
                auto box = new Box();

                box->text = this->getPropValue(node, "text");
                if (!box->text.empty()) replaceInString(box->text, "\\n", "\n");

                std::string sId = this->getPropValue(node, "id");
                if (sId.empty()) sId = this->getFallbackId();

                std::string rootMargin = this->getPropValue(node, "rootMargin");
                if (!rootMargin.empty()) box->rootMargin = std::stoi(rootMargin);

                std::string padding = this->getPropValue(node, "padding");
                if (!padding.empty()) box->padding = std::stoi(padding);

                std::string childrenPadding = this->getPropValue(node, "childrenPadding");
                if (!childrenPadding.empty()) box->childrenPadding = std::stoi(childrenPadding);

                std::string linkPadding = this->getPropValue(node, "linkPadding");
                if (!linkPadding.empty()) box->linkPadding = std::stoi(linkPadding);

                this->boxIdMap[sId] = box;
                box->children = this->processNodes(node->children, sId);
                boxes.push_back(box);
            }
        }

        return boxes;
    }

    std::string XmlLoader::getPropValue(xmlNode* node, std::string name)
    {
        xmlChar* value = xmlGetProp(node, (const xmlChar*)name.c_str());
        if (value != nullptr) {
            return (const char*)value;
        }
        return "";
    }

}
