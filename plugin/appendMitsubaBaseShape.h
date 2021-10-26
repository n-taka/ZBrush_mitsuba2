#ifndef APPEND_MITSUBA_BASE_SHAPE_H
#define APPEND_MITSUBA_BASE_SHAPE_H

#include "rapidxml.hpp"
#include "nlohmann/json.hpp"

using namespace rapidxml;

void appendMitsubaBaseShape(xml_document<> &doc, xml_node<> *scene, const nlohmann::json &parameters);

#endif
