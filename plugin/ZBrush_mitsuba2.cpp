#ifndef ZBRUSH_MITSUBA2_CPP
#define ZBRUSH_MITSUBA2_CPP

#include "ZBrush_mitsuba2.h"
#include "nlohmann/json.hpp"
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

#include <string>
#include <sstream>
#include <fstream>

// debug
#include <iostream>
// debug

#if defined(_WIN32) || defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#endif

#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace
{
	std::string getCurrentTimestampAsString()
	{
		time_t t = time(nullptr);
		const tm *localTime = localtime(&t);
		std::stringstream s;
		s << "20" << localTime->tm_year - 100;
		s << std::setw(2) << std::setfill('0') << localTime->tm_mon + 1;
		s << std::setw(2) << std::setfill('0') << localTime->tm_mday;
		s << std::setw(2) << std::setfill('0') << localTime->tm_hour;
		s << std::setw(2) << std::setfill('0') << localTime->tm_min;
		s << std::setw(2) << std::setfill('0') << localTime->tm_sec;
		return s.str();
	}
}

extern "C" DLLEXPORT float render(char *someText, double optValue, char *outputBuffer, int optBuffer1Size, char *pOptBuffer2, int optBuffer2Size, char **zData)
{
	////
	// [IN]
	// parameters = {
	// 	"root": "path/to/root/directory/of/this/plugin",
	// 	"format": "jpg"|"jpeg"|"png"|"bmp"|"tga",
	// 	"sensor": { # we always use thinlens
	//   "aperture_radius": size_of_aperture_radius,
	//   "focus_distance": distance_to_focal_plane,
	//   "near_clip": distance_to_clipping_near,
	//   "far_clip": distance_to_clipping_far,
	//   "sampler": {
	//    "type": "independent"|"stratified"|"multijitter"|"orthogonal"|"ldsampler",
	//    "sample_count": number_of_samples_per_pixel,
	//   },
	//   "film": { # we always use hdr (rgbe) format for rendering
	//    "width": number_of_pixels_for_width,
	//    "height": number_of_pixels_for_height,
	//    "rfilter": "box"|"tent"|"gaussian"|"mitchell"|"catmullrom"|"lanczos"
	//   }
	//  }
	//  TODO!!!!
	// }

	// [OUT]
	// nothing. we open the folder that contains the rendered image.

	////
	// parse parameter (JSON)
	std::string jsonString(someText);
	const nlohmann::json json = nlohmann::json::parse(jsonString);
	const std::string &rootString = json.at("root").get<std::string>();
	const fs::path rootPath(rootString);

	////
	// setup xml
	fs::path xmlPath(rootPath);
	fs::path exrPath(rootPath);
	const std::string timeStamp = getCurrentTimestampAsString();
	{
		std::string xmlFileName(timeStamp);
		xmlFileName += ".xml";
		xmlPath.append(xmlFileName);
		std::string exrFileName(timeStamp);
		exrFileName += ".exr";
		exrPath.append(exrFileName);

		using namespace rapidxml;

		// document root
		xml_document<> doc;
		{
			// scene
			xml_node<> *scene = doc.allocate_node(node_element, "scene");
			doc.append_node(scene);
			xml_attribute<> *sceneAttr = doc.allocate_attribute("version", "2.0.0");
			scene->append_attribute(sceneAttr);
			{
				// integrator
				xml_node<> *integrator = doc.allocate_node(node_element, "integrator");
				scene->append_node(integrator);
				xml_attribute<> *integratorAttr = doc.allocate_attribute("type", "path");
				integrator->append_attribute(integratorAttr);
			}
			{
				// sensor
				xml_node<> *sensor = doc.allocate_node(node_element, "sensor");
				scene->append_node(sensor);
				xml_attribute<> *sensorAttr = doc.allocate_attribute("type", "perspective");
				sensor->append_attribute(sensorAttr);
				{
					// TODO!!
				}
			}
			{
				// TODO!!
			}
		}
		// here, we render in HDR format (because STB image supports HDR image!)

		std::cout << doc << std::endl;
		return 1.0;

		// write to file
		std::ofstream xml(xmlPath);
		const std::string declaration("<?xml version=\"1.0\"?>");
		xml << declaration;
		xml << doc;
	}

	////
	// call mitsuba executable
	{
		fs::path mitsubaPath(rootPath);
		mitsubaPath.append("resources");
		mitsubaPath.append("mitsuba2");
		fs::path mitsubaExePath(mitsubaPath);
		mitsubaPath.append("mitsuba.exe");

		// setup command
		std::stringstream cmd;
		cmd << "\"";
		cmd << mitsubaExePath;
		cmd << " ";
		if (json.contains("variant"))
		{
			cmd << "-m ";
			cmd << json.at("variant").get<std::string>();
			cmd << " ";
		}
		cmd << xmlPath;
		cmd << "\"";
		system(cmd.str().c_str());
	}

	////
	// convert image to specified format
	if (json.contains("format"))
	{
		const std::string format = json.at("format").get<std::string>();

		if (format != "OpenEXR")
		{
			float *out;
			int width, height;
			const char *err = NULL;
			LoadEXR(&out, &width, &height, exrPath.string().c_str(), &err);

			std::vector<unsigned char> image;
			const int channels = 4;
			image.resize(width * height * channels);

			const auto hdrToldr = [](const float &hdrPixelValue, const float &gamma)
			{
				int i = static_cast<int>(255.0f * std::powf(hdrPixelValue, 1.0f / gamma));
				if (i > 255)
				{
					i = 255;
				}
				if (i < 0)
				{
					i = 0;
				}
				return static_cast<unsigned char>(i);
			};

			// default scale: 1.0
			// default gamma: 2.2
			// see: https://github.com/syoyo/tinyexr/blob/master/examples/exr2ldr/exr2ldr.cc
			const float scale = 1.0f;
			for (int h = 0; h < height; ++h)
			{
				for (int w = 0; w < width; ++w)
				{
					for (int c = 0; c < channels; ++c)
					{
						const int index = c + channels * (w + width * h);
						image.at(index) = hdrToldr(out[index] * scale, (c == 3 ? 1.0 : 2.2));
					}
				}
			}

			fs::path imagePath(rootPath);
			std::string imageFileName(timeStamp);
			imageFileName += ".";
			imageFileName += format;
			imagePath.append(imageFileName);
			if (format == "jpg" || format == "jpeg")
			{
				// currently, quality == 90
				stbi_write_jpg(imagePath.string().c_str(), width, height, channels, &(image[0]), 90);
			}
			else if (format == "png")
			{
				stbi_write_png(imagePath.string().c_str(), width, height, channels, &(image[0]), sizeof(unsigned char) * width * channels);
			}
			else if (format == "bmp")
			{
				stbi_write_bmp(imagePath.string().c_str(), width, height, channels, &(image[0]));
			}
			else if (format == "tga")
			{
				stbi_write_tga(imagePath.string().c_str(), width, height, channels, &(image[0]));
			}
		}
	}

	////
	// open folder
	// todo

	return 0.0;
}

#endif
