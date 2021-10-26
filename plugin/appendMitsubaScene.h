#ifndef APPEND_MITSUBA_SCENE_H
#define APPEND_MITSUBA_SCENE_H

#include "rapidxml.hpp"
#include "nlohmann/json.hpp"

using namespace rapidxml;

// parameters = {
//     "variant": "variant_for_rendering",
//     "sensor": { # we always use thinlens
//         "aperture_radius": size_of_aperture_radius,
//         "fov": fov_value,
//         "focus_distance": distance_to_focal_plane [-1.0, 1.0], // -1.0 => closest point on BSphere, 1.0 => fathest point on BSphere
//         "near_clip": distance_to_clipping_near,
//         "far_clip": distance_to_clipping_far,
//         "sampler": {
//             "type": "independent"|"stratified"|"multijitter"|"orthogonal"|"ldsampler",
//             "sample_count": number_of_samples_per_pixel,
//         },
//         "film": { # we always use exr format (default format for mitsuba) for rendering
//             "width": number_of_pixels_for_width,
//             "height": number_of_pixels_for_height,
//             "rfilter": "box"|"tent"|"gaussian"|"mitchell"|"catmullrom"|"lanczos"
//         },
//         "emitters": {
//             "constant": radiance_value_of_constant_emitter
//         }
//     }
// }

void appendMitsubaScene(xml_document<> &doc, nlohmann::json &parameters);

#endif
