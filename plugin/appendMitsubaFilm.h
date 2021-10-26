#ifndef APPEND_MITSUBA_FILM_H
#define APPEND_MITSUBA_FILM_H

#include "rapidxml.hpp"
#include "nlohmann/json.hpp"

using namespace rapidxml;

// [IN]
// parameters = {
//     "width" : number_of_pixels_for_width,
//     "height" : number_of_pixels_for_height,
//     "rfilter" : "box" | "tent" | "gaussian" | "mitchell" | "catmullrom" | "lanczos"
// }

void appendMitsubaFilm(xml_document<> &doc, xml_node<> *sensor, const nlohmann::json &parameters);

#endif
