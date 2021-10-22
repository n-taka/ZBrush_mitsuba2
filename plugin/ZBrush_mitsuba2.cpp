#ifndef ZBRUSH_MITSUBA2_CPP
#define ZBRUSH_MITSUBA2_CPP

#include "ZBrush_mitsuba2.h"
#include "nlohmann/json.hpp"
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

#include <string>
#include <sstream>
#include <fstream>

#include "readGoZFile.h"
#include "igl/write_triangle_mesh.h"

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
	// 	"mitsuba": {
	// 	 "variant": "variant_for_rendering",
	// 	 "sensor": { # we always use thinlens
	//    "aperture_radius": size_of_aperture_radius,
	//    "focal_length": "focal_length",
	//    "fov": fov_value,
	//    "focus_distance": distance_to_focal_plane,
	//    "near_clip": distance_to_clipping_near,
	//    "far_clip": distance_to_clipping_far,
	//    "sampler": {
	//     "type": "independent"|"stratified"|"multijitter"|"orthogonal"|"ldsampler",
	//     "sample_count": number_of_samples_per_pixel,
	//    },
	//    "film": { # we always use exr format (default format for mitsuba) for rendering
	//     "width": number_of_pixels_for_width,
	//     "height": number_of_pixels_for_height,
	//     "rfilter": "box"|"tent"|"gaussian"|"mitchell"|"catmullrom"|"lanczos"
	//    },
	//    "emitters": {
	//     "constant": radiance_value_of_constant_emitter
	//    }
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
	const std::string timeStamp = getCurrentTimestampAsString();

	std::cout << json.dump(4) << std::endl;

	////
	// calculate rotation of the mesh
	//   Here we load GoZ file format (because such data contains complete information)
	//   Then we export rotated mesh in ply format
	Eigen::Matrix<double, 1, Eigen::Dynamic> center;
	std::string meshName;
	{
		const std::string &meshFileRelPathStr = json.at("meshFile").get<std::string>();
		fs::path meshFileRelPath(meshFileRelPathStr);
		fs::path meshFilePath(rootPath);
		meshFilePath /= meshFileRelPath;

		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> V;
		Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> F;
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> VC;
		{
			// igl::read_triangle_mesh(meshFilePath.string(), V, F);
			std::vector<std::vector<double>> VVec;
			std::vector<std::vector<int>> FVec;
			std::vector<std::vector<std::pair<double, double>>> UVVec;
			std::vector<std::vector<double>> VCVec;
			std::vector<double> MVec;
			std::vector<int> GVec;
			// todo tweak for vertex colors
			FromZ::readGoZFile(meshFilePath.string(), meshName, VVec, FVec, UVVec, VCVec, MVec, GVec);
			int triCount = 0;
			for (const auto &f : FVec)
			{
				triCount += (f.size() - 2);
			}
			V.resize(VVec.size(), 3);
			for (int v = 0; v < VVec.size(); ++v)
			{
				V.row(v) << VVec.at(v).at(0), VVec.at(v).at(1), VVec.at(v).at(2);
			}
			VC.resize(VCVec.size(), 4);
			for (int vc = 0; vc < VCVec.size(); ++vc)
			{
				VC.row(vc) << VCVec.at(vc).at(0), VCVec.at(vc).at(1), VCVec.at(vc).at(2);
			}
			F.resize(triCount, 3);
			int fIdx = 0;
			for (int f = 0; f < FVec.size(); ++f)
			{
				for (int tri = 0; tri < FVec.at(f).size() - 2; ++tri)
				{
					F.row(fIdx) << FVec.at(f).at(0), FVec.at(f).at(1 + tri), FVec.at(f).at(2 + tri);
					fIdx++;
				}
			}
		}

		// scale / translate
		//   In our setting, we scale so that its minimum bounding sphere has radius of 50.
		//   * We use sphere so that sphere is rotational symmetry
		{
			// rotate
			// change cooridnate system in ZBrush => mitsuba
			//   rotate 180 degree around Z-axis
			V.col(1) *= -1;
			V.col(2) *= -1;

			// scale
			Eigen::Matrix<double, 3, Eigen::Dynamic> XP;
			XP.resize(3, V.cols());
			center.resize(1, V.cols());
			double radius = 0;

			const auto updateSphereFromXP = [&XP, &center, &radius]()
			{
				const double a = (XP.row(2) - XP.row(1)).norm();
				const double b = (XP.row(0) - XP.row(2)).norm();
				const double c = (XP.row(1) - XP.row(0)).norm();
				const double cosA = (XP.row(1) - XP.row(0)).dot(XP.row(2) - XP.row(0)) / (b * c);
				const double cosB = (XP.row(0) - XP.row(1)).dot(XP.row(2) - XP.row(1)) / (c * a);
				const double cosC = (XP.row(0) - XP.row(2)).dot(XP.row(1) - XP.row(2)) / (a * b);
				center = (a * cosA * XP.row(0) + b * cosB * XP.row(1) + c * cosC * XP.row(2)) / (a * cosA + b * cosB + c * cosC);
				radius = (center - XP.row(0)).norm();
			};
			// initialize pick first 3 points to form a minimum bounding sphere
			XP.row(0) = V.row(0);
			XP.row(1) = V.row(1);
			XP.row(2) = V.row(2);
			updateSphereFromXP();
			for (int v = 3; v < V.rows(); ++v)
			{
				const double distToCenter = (V.row(v) - center).norm();
				if (distToCenter > radius)
				{
					// remove closest vertex
					double distToClosest = std::numeric_limits<double>::max();
					int closestXPIdx = -1;
					for (int xp = 0; xp < XP.rows(); ++xp)
					{
						const double distToNewVert = (XP.row(xp) - V.row(v)).norm();
						if (distToNewVert < distToClosest)
						{
							distToClosest = distToNewVert;
							closestXPIdx = xp;
						}
					}
					if (closestXPIdx > -1)
					{
						// update XP
						XP.row(closestXPIdx) = V.row(v);
						// update center/radius
						updateSphereFromXP();
					}
				}
			}
			V *= (50.0 / radius);
			// translate
			//   center of the bounding sphere is aligned on the Y axis
			//   the lowest position of the mesh is aligned on the ground (y=0)
			Eigen::Matrix<double, 1, Eigen::Dynamic> translation;
			translation.resize(1, 3);
			translation(0, 0) = -center(0, 0);
			translation(0, 1) = -V.col(1).minCoeff();
			translation(0, 2) = -center(0, 2);
			V.rowwise() += translation;
			center += translation;
		}
		fs::path meshPath(rootPath);
		meshPath.append("mesh.ply");
		// todo tweak for vertex colors
		igl::write_triangle_mesh(meshPath.string(), V, F);
	}

	////
	// setup xml
	fs::path xmlPath(rootPath);
	fs::path exrPath(rootPath);
	{
		std::string xmlFileName(meshName);
		xmlFileName += "_";
		xmlFileName += timeStamp;
		xmlFileName += ".xml";
		xmlPath.append(xmlFileName);
		std::string exrFileName(meshName);
		exrFileName += "_";
		exrFileName += timeStamp;
		exrFileName += ".exr";
		exrPath.append(exrFileName);

		using namespace rapidxml;

		// document root
		xml_document<> doc;
		{
			// scene
			xml_node<> *scene = doc.allocate_node(node_element, "scene");
			xml_attribute<> *sceneAttr = doc.allocate_attribute("version", "2.0.0");
			scene->append_attribute(sceneAttr);
			{
				// integrator
				//   currently, we only support path tracer
				xml_node<> *integrator = doc.allocate_node(node_element, "integrator");
				xml_attribute<> *integratorAttr = doc.allocate_attribute("type", "path");
				integrator->append_attribute(integratorAttr);
				scene->append_node(integrator);
			}
			{
				// sensor
				const nlohmann::json &sensorJson = json.at("mitsuba").at("sensor");

				xml_node<> *sensor = doc.allocate_node(node_element, "sensor");
				xml_attribute<> *sensorAttr = doc.allocate_attribute("type", "thinlens");
				sensor->append_attribute(sensorAttr);
				{
					// transform
					xml_node<> *transform = doc.allocate_node(node_element, "transform");
					xml_attribute<> *transformAttr = doc.allocate_attribute("name", "to_world");
					transform->append_attribute(transformAttr);
					{
						// mitsuba2 is Y-up coordinate
						xml_node<> *lookat = doc.allocate_node(node_element, "lookat");
						char buf[255];
						snprintf(buf, 255, "%f, %f, %f", center(0, 0), center(0, 1), center(0, 2));
						char *target_value = doc.allocate_string(buf);
						xml_attribute<> *targetAttr = doc.allocate_attribute("target", target_value);
						lookat->append_attribute(targetAttr);
						// todo tweak sensor position based on camera angle in ZBrush
						snprintf(buf, 255, "%f, %f, %f", center(0, 0), center(0, 1), center(0, 2) + 100);
						char *origin_value = doc.allocate_string(buf);
						xml_attribute<> *originAttr = doc.allocate_attribute("origin", origin_value);
						lookat->append_attribute(originAttr);
						xml_attribute<> *upAttr = doc.allocate_attribute("up", "0.0, 1.0, 0.0");
						lookat->append_attribute(upAttr);
						transform->append_node(lookat);
					}
					sensor->append_node(transform);
				}
				{
					// aperture_radius
					xml_node<> *aperture_radius = doc.allocate_node(node_element, "float");
					xml_attribute<> *aperture_radius_nameAttr = doc.allocate_attribute("name", "aperture_radius");
					aperture_radius->append_attribute(aperture_radius_nameAttr);
					char buf[255];
					snprintf(buf, 255, "%f", sensorJson.at("aperture_radius").get<float>());
					char *aperture_radius_value = doc.allocate_string(buf);
					xml_attribute<> *aperture_radius_valueAttr = doc.allocate_attribute("value", aperture_radius_value);
					aperture_radius->append_attribute(aperture_radius_valueAttr);
					sensor->append_node(aperture_radius);
				}
				{
					// focus_distance
					// todo: convert parameter to actual length
					xml_node<> *focus_distance = doc.allocate_node(node_element, "float");
					xml_attribute<> *focus_distance_nameAttr = doc.allocate_attribute("name", "focus_distance");
					focus_distance->append_attribute(focus_distance_nameAttr);
					char buf[255];
					snprintf(buf, 255, "%f", sensorJson.at("focus_distance").get<float>());
					char *focus_distance_value = doc.allocate_string(buf);
					xml_attribute<> *focus_distance_valueAttr = doc.allocate_attribute("value", focus_distance_value);
					focus_distance->append_attribute(focus_distance_valueAttr);
					sensor->append_node(focus_distance);
				}
				if (sensorJson.contains("focal_length"))
				{
					// focal_length
					xml_node<> *focal_length = doc.allocate_node(node_element, "string");
					xml_attribute<> *focal_length_nameAttr = doc.allocate_attribute("name", "focal_length");
					focal_length->append_attribute(focal_length_nameAttr);
					char *value = doc.allocate_string(sensorJson.at("focal_length").get<std::string>().c_str());
					xml_attribute<> *focal_length_valueAttr = doc.allocate_attribute("value", value);
					focal_length->append_attribute(focal_length_valueAttr);
					sensor->append_node(focal_length);
				}
				else if (sensorJson.contains("fov"))
				{
					// fov
					xml_node<> *fov = doc.allocate_node(node_element, "float");
					xml_attribute<> *fov_nameAttr = doc.allocate_attribute("name", "fov");
					fov->append_attribute(fov_nameAttr);
					char buf[255];
					snprintf(buf, 255, "%f", sensorJson.at("fov").get<float>());
					char *fov_value = doc.allocate_string(buf);
					xml_attribute<> *fov_valueAttr = doc.allocate_attribute("value", fov_value);
					fov->append_attribute(fov_valueAttr);
					sensor->append_node(fov);
				}
				{
					// fov_axis
					xml_node<> *fov_axis = doc.allocate_node(node_element, "string");
					xml_attribute<> *fov_axis_nameAttr = doc.allocate_attribute("name", "fov_axis");
					fov_axis->append_attribute(fov_axis_nameAttr);
					// fov_axis: x is default value, but we explicitly set to "x" (possibly, this could be updated later...)
					xml_attribute<> *fov_axis_valueAttr = doc.allocate_attribute("value", "x");
					fov_axis->append_attribute(fov_axis_valueAttr);
					sensor->append_node(fov_axis);
				}
				{
					// near_clip
					xml_node<> *near_clip = doc.allocate_node(node_element, "float");
					xml_attribute<> *near_clip_nameAttr = doc.allocate_attribute("name", "near_clip");
					near_clip->append_attribute(near_clip_nameAttr);
					char buf[255];
					snprintf(buf, 255, "%f", sensorJson.at("near_clip").get<float>());
					char *near_clip_value = doc.allocate_string(buf);
					xml_attribute<> *near_clip_valueAttr = doc.allocate_attribute("value", near_clip_value);
					near_clip->append_attribute(near_clip_valueAttr);
					sensor->append_node(near_clip);
				}
				{
					// far_clip
					xml_node<> *far_clip = doc.allocate_node(node_element, "float");
					xml_attribute<> *far_clip_nameAttr = doc.allocate_attribute("name", "far_clip");
					far_clip->append_attribute(far_clip_nameAttr);
					char buf[255];
					snprintf(buf, 255, "%f", sensorJson.at("far_clip").get<float>());
					char *far_clip_value = doc.allocate_string(buf);
					xml_attribute<> *far_clip_valueAttr = doc.allocate_attribute("value", far_clip_value);
					far_clip->append_attribute(far_clip_valueAttr);
					sensor->append_node(far_clip);
				}

				{
					// sampler
					const nlohmann::json &samplerJson = sensorJson.at("sampler");
					xml_node<> *sampler = doc.allocate_node(node_element, "sampler");
					char *type_value = doc.allocate_string(samplerJson.at("type").get<std::string>().c_str());
					xml_attribute<> *samplerAttr = doc.allocate_attribute("type", type_value);
					sampler->append_attribute(samplerAttr);
					{
						xml_node<> *sample_count = doc.allocate_node(node_element, "integer");
						{
							xml_attribute<> *sample_countAttr = doc.allocate_attribute("name", "sample_count");
							sample_count->append_attribute(sample_countAttr);
						}
						{
							char buf[255];
							snprintf(buf, 255, "%d", samplerJson.at("sample_count").get<int>());
							char *sample_count_value = doc.allocate_string(buf);
							xml_attribute<> *sample_countAttr = doc.allocate_attribute("value", sample_count_value);
							sample_count->append_attribute(sample_countAttr);
						}
						sampler->append_node(sample_count);
					}
					sensor->append_node(sampler);
				}
				{
					// film
					const nlohmann::json &filmJson = sensorJson.at("film");
					xml_node<> *film = doc.allocate_node(node_element, "film");
					xml_attribute<> *filmAttr = doc.allocate_attribute("type", "hdrfilm");
					film->append_attribute(filmAttr);
					{
						xml_node<> *width = doc.allocate_node(node_element, "integer");
						{
							xml_attribute<> *widthAttr = doc.allocate_attribute("name", "width");
							width->append_attribute(widthAttr);
						}
						{
							char buf[255];
							snprintf(buf, 255, "%d", filmJson.at("width").get<int>());
							char *width_value = doc.allocate_string(buf);
							xml_attribute<> *widthAttr = doc.allocate_attribute("value", width_value);
							width->append_attribute(widthAttr);
						}
						film->append_node(width);
					}
					{
						xml_node<> *height = doc.allocate_node(node_element, "integer");
						{
							xml_attribute<> *heightAttr = doc.allocate_attribute("name", "height");
							height->append_attribute(heightAttr);
						}
						{
							char buf[255];
							snprintf(buf, 255, "%d", filmJson.at("height").get<int>());
							char *height_value = doc.allocate_string(buf);
							xml_attribute<> *heightAttr = doc.allocate_attribute("value", height_value);
							height->append_attribute(heightAttr);
						}
						film->append_node(height);
					}
					{
						xml_node<> *rfilter = doc.allocate_node(node_element, "rfilter");
						char *type_value = doc.allocate_string(filmJson.at("rfilter").get<std::string>().c_str());
						xml_attribute<> *rfilterAttr = doc.allocate_attribute("type", type_value);
						rfilter->append_attribute(rfilterAttr);
						film->append_node(rfilter);
					}
					sensor->append_node(film);
				}
				scene->append_node(sensor);
			}
			{
				// emitters
				const nlohmann::json &emitterJson = json.at("mitsuba").at("emitters");

				{
					// emitter 0: constant
					xml_node<> *emitter = doc.allocate_node(node_element, "emitter");
					xml_attribute<> *emitterAttr = doc.allocate_attribute("type", "constant");
					emitter->append_attribute(emitterAttr);
					{
						xml_node<> *spectrum = doc.allocate_node(node_element, "spectrum");
						xml_attribute<> *spectrumAttr = doc.allocate_attribute("name", "radiance");
						spectrum->append_attribute(spectrumAttr);
						char buf[255];
						snprintf(buf, 255, "%f", emitterJson.at("constant").get<float>());
						char *radiance_value = doc.allocate_string(buf);
						xml_attribute<> *radiance_valueAttr = doc.allocate_attribute("value", radiance_value);
						spectrum->append_attribute(radiance_valueAttr);

						emitter->append_node(spectrum);
					}
					scene->append_node(emitter);
				}

				// emitter 1: area light
				// todo

				// emitter 2: directional light
				// todo
			}
			{
				// floor/back
				//   if camera views from above
				// TODO!!
			} {
				// back
				//   if camera views from below
				// TODO!!
			} {
				// target shape
				// TODO!!
			}
			doc.append_node(scene);
		}
		std::cout << doc << std::endl;

		//     <bsdf type="diffuse" id="bsdf-diffuse">
		//         <rgb name="reflectance" value="0.18 0.18 0.18" />
		//     </bsdf>

		//     <texture type="checkerboard" id="texture-checkerboard">
		//         <rgb name="color0" value="0.4" />
		//         <rgb name="color1" value="0.2" />
		//         <transform name="to_uv">
		//             <scale x="8.000000" y="8.000000" />
		//         </transform>
		//     </texture>

		//     <bsdf type="diffuse" id="bsdf-plane">
		//         <ref name="reflectance" id="texture-checkerboard" />
		//     </bsdf>

		//     <bsdf type="plastic" id="bsdf-matpreview">
		//         <rgb name="diffuse_reflectance" value="0.940, 0.271, 0.361" />
		//         <float name="int_ior" value="1.9" />
		//     </bsdf>

		//     <shape type="serialized" id="shape-plane">
		//         <string name="filename" value="matpreview.serialized" />
		//         <integer name="shape_index" value="0" />
		//         <transform name="to_world">
		//             <rotate z="1" angle="-4.3" />
		//             <matrix value="3.38818 -4.06354 0 -1.74958 4.06354 3.38818 0 1.43683 0 0 5.29076 -0.0120714 0 0 0 1" />
		//         </transform>
		//         <ref name="bsdf" id="bsdf-plane" />
		//     </shape>

		//     <shape type="serialized" id="shape-matpreview-interior">
		//         <string name="filename" value="matpreview.serialized" />
		//         <integer name="shape_index" value="1" />
		//         <transform name="to_world">
		//             <matrix value="1 0 0 0 0 1 0 0 0 0 1 0.0252155 0 0 0 1" />
		//         </transform>
		//         <ref name="bsdf" id="bsdf-diffuse" />
		//     </shape>

		//     <shape type="serialized" id="shape-matpreview-exterior">
		//         <string name="filename" value="matpreview.serialized" />
		//         <integer name="shape_index" value="2" />
		//         <transform name="to_world">
		//             <matrix value="0.614046 0.614047 0 -1.78814e-07 -0.614047 0.614046 0 2.08616e-07 0 0 0.868393 1.02569 0 0 0 1" />
		//             <translate z="0.01" />
		//         </transform>

		//         <ref name="bsdf" id="bsdf-matpreview" />
		//     </shape>

		// write to file
		std::ofstream xml(xmlPath);
		const std::string declaration("<?xml version=\"1.0\"?>");
		xml << declaration;
		xml << doc;
	}

	return 0.0;

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
		if (json.at("mitsuba").contains("variant"))
		{
			cmd << "-m ";
			cmd << json.at("mitsuba").at("variant").get<std::string>();
			cmd << " ";
		}
		cmd << xmlPath;
		cmd << "\"";

		std::cout << cmd.str() << std::endl;

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
