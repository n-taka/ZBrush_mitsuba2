#ifndef APPEND_MITSUBA_TARGET_SHAPE_H
#define APPEND_MITSUBA_TARGET_SHAPE_H

#include "rapidxml.hpp"
#include "nlohmann/json.hpp"

using namespace rapidxml;

void appendMitsubaTargetShape(xml_document<> &doc, xml_node<> *scene, const nlohmann::json &parameters);

#endif
