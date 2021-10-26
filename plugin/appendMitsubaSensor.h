#ifndef APPEND_MITSUBA_SENSOR_H
#define APPEND_MITSUBA_SENSOR_H

#include "rapidxml.hpp"
#include "nlohmann/json.hpp"

using namespace rapidxml;

void appendMitsubaSensor(xml_document<> &doc, xml_node<> *scene, nlohmann::json &parameters);

#endif
