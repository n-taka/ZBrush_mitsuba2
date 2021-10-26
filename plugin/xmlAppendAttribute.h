#ifndef XML_APPEND_ATTRIBUTE_H
#define XML_APPEND_ATTRIBUTE_H

#include <string>
#include <sstream>
#include "rapidxml.hpp"

using namespace rapidxml;

template <typename T>
void xmlAppendAttribute(xml_document<> &doc, xml_node<> *node, const std::string &name, const T &value)
{
    std::stringstream ssName, ssValue;
    ssName << name;
    ssValue << value;

    char *charName = doc.allocate_string(ssName.str().c_str());
    char *charValue = doc.allocate_string(ssValue.str().c_str());
    xml_attribute<> *xmlAttr = doc.allocate_attribute(charName, charValue);

    node->append_attribute(xmlAttr);
}

#endif
