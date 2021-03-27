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

}
