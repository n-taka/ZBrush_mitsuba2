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
        json["meshFile"] = "data/HeadColor.GoZ";
        json["mitsuba"] = nlohmann::json::object();
        json["mitsuba"]["variant"] = "scalar_spectral";
        json["mitsuba"]["sensor"] = nlohmann::json::object();
        json["mitsuba"]["sensor"]["aperture_radius"] = 0.1;
        json["mitsuba"]["sensor"]["focus_distance"] = 0.0;
        json["mitsuba"]["sensor"]["fov"] = 15.0;
        json["mitsuba"]["sensor"]["near_clip"] = 0.01;
        json["mitsuba"]["sensor"]["far_clip"] = 10000;
        json["mitsuba"]["sensor"]["sampler"] = nlohmann::json::object();
        json["mitsuba"]["sensor"]["sampler"]["type"] = "multijitter";
        json["mitsuba"]["sensor"]["sampler"]["sample_count"] = 64;
        json["mitsuba"]["sensor"]["film"] = nlohmann::json::object();
        json["mitsuba"]["sensor"]["film"]["width"] = 640;
        json["mitsuba"]["sensor"]["film"]["height"] = 360;
        json["mitsuba"]["sensor"]["film"]["rfilter"] = "lanczos";
        json["mitsuba"]["sensor"]["marginSize"] = 0.05;
        json["mitsuba"]["sensor"]["viewDir"]["x"] = 0.0;
        json["mitsuba"]["sensor"]["viewDir"]["y"] = 0.0;
        json["mitsuba"]["sensor"]["viewDir"]["z"] = -1.0;
        json["mitsuba"]["sensor"]["viewRight"]["x"] = 1.0;
        json["mitsuba"]["sensor"]["viewRight"]["y"] = 0.0;
        json["mitsuba"]["sensor"]["viewRight"]["z"] = 0.0;
        json["mitsuba"]["sensor"]["viewUp"]["x"] = 0.0;
        json["mitsuba"]["sensor"]["viewUp"]["y"] = 1.0;
        json["mitsuba"]["sensor"]["viewUp"]["z"] = 0.0;
        json["mitsuba"]["emitters"] = nlohmann::json::object();
        json["mitsuba"]["emitters"]["constant"] = 0.5;
        json["mitsuba"]["emitters"]["area"] = 100.0;
        json["mitsuba"]["shapes"]["background"] = nlohmann::json::object();
        json["mitsuba"]["shapes"]["background"]["r"] = 255.0f * 0.2f;
        json["mitsuba"]["shapes"]["background"]["g"] = 255.0f * 0.25f;
        json["mitsuba"]["shapes"]["background"]["b"] = 255.0f * 0.7f;
        json["export"]["format"] = "png";
        json["export"]["scale"] = 1.0;
        json["export"]["gamma"] = 2.2;
        json["export"]["openDirectory"] = true;
        sprintf(buf, "%s", json.dump().c_str());
    }
    // float render(char *someText, double optValue, char *outputBuffer, int optBuffer1Size, char *pOptBuffer2, int optBuffer2Size, char **zData);
    char **zData;
    render(buf, 0.0, buf, 0, buf, 0, zData);

    return;
}
