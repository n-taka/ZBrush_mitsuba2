#ifndef APPEND_MITSUBA_EMITTER_H
#define APPEND_MITSUBA_EMITTER_H

#include "rapidxml.hpp"
#include "nlohmann/json.hpp"

using namespace rapidxml;

void appendMitsubaEmitter(xml_document<> &doc, xml_node<> *scene, const nlohmann::json &parameters);

#endif
