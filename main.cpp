#include <iostream>
#include <string>
#include "nlohmann/json.hpp"
#include "../plugin/ZBrush_mitsuba2.h"

void main()
{
    char buf[255];
    {
        nlohmann::json json;
        json["root"] = "./build/src/Release/";
        json["variant"] = "scalar_spectral";
        json["format"] = "png";
        sprintf(buf, "%s", json.dump().c_str());
    }
    // float render(char *someText, double optValue, char *outputBuffer, int optBuffer1Size, char *pOptBuffer2, int optBuffer2Size, char **zData);
    char **zData;
    render(buf, 0.0, buf, 0, buf, 0, zData);

    return;
}
