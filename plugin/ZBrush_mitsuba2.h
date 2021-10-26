#ifndef ZBRUSH_MITSUBA2_H
#define ZBRUSH_MITSUBA2_H

#if defined(_WIN32) || defined(_WIN64)
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __attribute__((visibility("default")))
#endif

////
// [IN]
// parameters = {
// 	   "root": "path/to/root/directory/of/this/plugin",
// 	   "meshFile": "relpath of GoZ file, the mesh is, <root>/<meshFile>",
// 	   "format": "jpg"|"jpeg"|"png"|"bmp"|"tga",
//     "background": {
//      "r": [0, 255],
//      "g": [0, 255],
//      "b": [0, 255],
//     }
// 	   "mitsuba": {
// 	    "variant": "variant_for_rendering",
// 	    "sensor": { # we always use thinlens
// 	     "marginSize": margin_ratio [0.0, inf), // this parameter only guarantee that "at least", depends on fov value
//       "aperture_radius": size_of_aperture_radius,
//       "fov": fov_value,
//       "focus_distance": distance_to_focal_plane [-1.0, 1.0], // -1.0 => closest point on BSphere, 1.0 => fathest point on BSphere
//       "near_clip": distance_to_clipping_near,
//       "far_clip": distance_to_clipping_far,
//       "sampler": {
//        "type": "independent"|"stratified"|"multijitter"|"orthogonal"|"ldsampler",
//        "sample_count": number_of_samples_per_pixel,
//       },
//       "film": { # we always use exr format (default format for mitsuba) for rendering
//        "width": number_of_pixels_for_width,
//        "height": number_of_pixels_for_height,
//        "rfilter": "box"|"tent"|"gaussian"|"mitchell"|"catmullrom"|"lanczos"
//       },
//       "emitters": {
//        "constant": radiance_value_of_constant_emitter
//       }
//      }
//     }
//     TODO!!!!
// }

// [OUT]
// nothing. we open the folder that contains the rendered image.

extern "C" DLLEXPORT float render(char *someText, double optValue, char *outputBuffer, int optBuffer1Size, char *pOptBuffer2, int optBuffer2Size, char **zData);

#endif
