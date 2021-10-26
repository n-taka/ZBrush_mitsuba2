#ifndef APPEND_MITSUBA_SAMPLER_H
#define APPEND_MITSUBA_SAMPLER_H

#include "rapidxml.hpp"
#include "nlohmann/json.hpp"

using namespace rapidxml;

// [IN]
// "sampler": {
//     "type": "independent"|"stratified"|"multijitter"|"orthogonal"|"ldsampler",
//     "sample_count": number_of_samples_per_pixel,
// }

void appendMitsubaSampler(xml_document<> &doc, xml_node<> *sensor, const nlohmann::json &parameters);

#endif
