#include <iostream>
#include <string>
#include "nlohmann/json.hpp"
#include "../plugin/ZBrush_mitsuba2.h"

void main()
{
    char buf[255];
    {
        nlohmann::json json;
        json["root"] = "./build/Release/";
        json["format"] = "png";
        json["meshFile"] = "data/Armadillo.ply";
        json["mitsuba"] = nlohmann::json::object();
        json["mitsuba"]["variant"] = "scalar_spectral";
        json["mitsuba"]["sensor"] = nlohmann::json::object();
        json["mitsuba"]["sensor"]["aperture_radius"] = 0.1;
        json["mitsuba"]["sensor"]["focus_distance"] = 0.2;
        json["mitsuba"]["sensor"]["focal_length"] = "50mm";
        json["mitsuba"]["sensor"]["near_clip"] = 0.01;
        json["mitsuba"]["sensor"]["far_clip"] = 10000;
        json["mitsuba"]["sensor"]["sampler"] = nlohmann::json::object();
        json["mitsuba"]["sensor"]["sampler"]["type"] = "ldsampler";
        json["mitsuba"]["sensor"]["sampler"]["sample_count"] = 64;
        json["mitsuba"]["sensor"]["film"] = nlohmann::json::object();
        json["mitsuba"]["sensor"]["film"]["width"] = 640;
        json["mitsuba"]["sensor"]["film"]["height"] = 360;
        json["mitsuba"]["sensor"]["film"]["rfilter"] = "lanczos";
        json["mitsuba"]["emitters"] = nlohmann::json::object();
        json["mitsuba"]["emitters"]["constant"] = 1.0;
        sprintf(buf, "%s", json.dump().c_str());
    }
    // float render(char *someText, double optValue, char *outputBuffer, int optBuffer1Size, char *pOptBuffer2, int optBuffer2Size, char **zData);
    char **zData;
    render(buf, 0.0, buf, 0, buf, 0, zData);

    return;
}
