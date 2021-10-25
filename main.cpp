#include <iostream>
#include <string>
#include "nlohmann/json.hpp"
#include "../plugin/ZBrush_mitsuba2.h"

void main()
{
    char buf[255];
    {
        nlohmann::json json;
        json["root"] = "./build/Release";
        json["format"] = "png";
        json["marginSize"] = 0.05;
        // json["meshFile"] = "PolySphere.GoZ";
        json["meshFile"] = "HeadColor.GoZ";
        json["background"] = nlohmann::json::object();
        json["background"]["r"] = 255.0f * 0.2f;
        json["background"]["g"] = 255.0f * 0.25f;
        json["background"]["b"] = 255.0f * 0.7f;
        json["mitsuba"] = nlohmann::json::object();
        json["mitsuba"]["variant"] = "scalar_spectral";
        json["mitsuba"]["sensor"] = nlohmann::json::object();
        json["mitsuba"]["sensor"]["aperture_radius"] = 0.1;
        json["mitsuba"]["sensor"]["focus_distance"] = 0.0;
        // json["mitsuba"]["sensor"]["focal_length"] = "200mm";
        json["mitsuba"]["sensor"]["fov"] = 15.0;
        // json["mitsuba"]["sensor"]["fov_axis"] = "x";
        json["mitsuba"]["sensor"]["near_clip"] = 0.01;
        json["mitsuba"]["sensor"]["far_clip"] = 10000;
        json["mitsuba"]["sensor"]["sampler"] = nlohmann::json::object();
        json["mitsuba"]["sensor"]["sampler"]["type"] = "multijitter";
        json["mitsuba"]["sensor"]["sampler"]["sample_count"] = 128;
        json["mitsuba"]["sensor"]["film"] = nlohmann::json::object();
        json["mitsuba"]["sensor"]["film"]["width"] = 640;
        json["mitsuba"]["sensor"]["film"]["height"] = 320;
        json["mitsuba"]["sensor"]["film"]["rfilter"] = "lanczos";
        json["mitsuba"]["emitters"] = nlohmann::json::object();
        json["mitsuba"]["emitters"]["constant"] = 0.5;
        json["mitsuba"]["emitters"]["area"] = 10.0;
        sprintf(buf, "%s", json.dump().c_str());
    }
    // float render(char *someText, double optValue, char *outputBuffer, int optBuffer1Size, char *pOptBuffer2, int optBuffer2Size, char **zData);
    char **zData;
    render(buf, 0.0, buf, 0, buf, 0, zData);

    return;
}
