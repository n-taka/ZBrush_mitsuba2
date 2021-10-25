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
#include "igl/per_vertex_normals.h"
#include "igl/writePLY.h"

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
	// 	"meshFile": "fileName of GoZ file, the mesh is, <root>/data/<meshFile>",
	// 	"format": "jpg"|"jpeg"|"png"|"bmp"|"tga",
	// 	"marginSize": margin_ratio [0.0, inf), // this parameter only guarantee that "at least", depends on fov value
	//  "background": {
	//   "r": [0, 255],
	//   "g": [0, 255],
	//   "b": [0, 255],
	//  }
	// 	"mitsuba": {
	// 	 "variant": "variant_for_rendering",
	// 	 "sensor": { # we always use thinlens
	//    "aperture_radius": size_of_aperture_radius,
	//    "fov": fov_value,
	//    "focus_distance": distance_to_focal_plane [-1.0, 1.0], // -1.0 => closest point on BSphere, 1.0 => fathest point on BSphere
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
	fs::path dataPath(rootPath);
	dataPath.append("data");
	if (!fs::exists(dataPath))
	{
		fs::create_directories(dataPath);
	}
	const std::string timeStamp = getCurrentTimestampAsString();

	std::cout << json.dump(4) << std::endl;

	////
	// calculate rotation of the mesh
	//   Here we load GoZ file format (because such data contains complete information)
	//   Then we export rotated mesh in ply format
	const double BSphereRadius = 50.0;
	Eigen::Matrix<double, 1, Eigen::Dynamic> center;
	std::string meshName;
	bool hasVC = false;
	{
		const std::string &meshFileRelPathStr = json.at("meshFile").get<std::string>();
		fs::path meshFileRelPath(meshFileRelPathStr);
		fs::path meshFilePath(dataPath);
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
				VC.row(vc) << VCVec.at(vc).at(0), VCVec.at(vc).at(1), VCVec.at(vc).at(2), VCVec.at(vc).at(3);
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
			V *= (BSphereRadius / radius);
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
		fs::path meshPath(dataPath);
		meshPath.append("mesh.ply");
		// todo tweak for vertex colors
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> N;
		igl::per_vertex_normals(V, F, N);
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> UV;
		UV.resize(0, 2);
		if (V.rows() == VC.rows())
		{
			hasVC = true;
			// we shouldn't add alpha channel (somehow mesh_attribute texture fails when we add alpha...)
			igl::writePLY(meshPath.string(), V, F, N, UV, VC.block(0, 0, VC.rows(), 3), std::vector<std::string>({std::string("r"), std::string("g"), std::string("b")}));
		}
		else
		{
			igl::writePLY(meshPath.string(), V, F, N, UV);
		}
	}

	////
	// setup xml
	fs::path xmlPath(dataPath);
	fs::path exrPath(dataPath);
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
			Eigen::Matrix<double, 1, Eigen::Dynamic> cameraPosition;
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
						cameraPosition = center;
						// todo
						// tweak sensor position based on camera angle in ZBrush
						Eigen::Matrix<double, 1, Eigen::Dynamic> unitVectorFromTargetToCamera;
						unitVectorFromTargetToCamera.resize(1, 3);
						unitVectorFromTargetToCamera << 0.0, 0.0, 1.0;
						const double BSphereRadiusWithMargin = (BSphereRadius * (1.0 + json.at("marginSize").get<float>()));
						const double dist = BSphereRadiusWithMargin / std::sin(0.5 * json.at("mitsuba").at("sensor").at("fov").get<double>() * M_PI / 180.0);
						cameraPosition += dist * (unitVectorFromTargetToCamera);
						xml_node<> *lookat = doc.allocate_node(node_element, "lookat");
						char buf[255];
						snprintf(buf, 255, "%f, %f, %f", center(0, 0), center(0, 1), center(0, 2));
						char *target_value = doc.allocate_string(buf);
						xml_attribute<> *targetAttr = doc.allocate_attribute("target", target_value);
						lookat->append_attribute(targetAttr);
						snprintf(buf, 255, "%f, %f, %f", cameraPosition(0, 0), cameraPosition(0, 1), cameraPosition(0, 2));
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
					double focusDist = (center - cameraPosition).norm();
					focusDist += (sensorJson.at("focus_distance").get<double>() * BSphereRadius);
					char buf[255];
					snprintf(buf, 255, "%f", static_cast<float>(focusDist));
					char *focus_distance_value = doc.allocate_string(buf);
					xml_attribute<> *focus_distance_valueAttr = doc.allocate_attribute("value", focus_distance_value);
					focus_distance->append_attribute(focus_distance_valueAttr);
					sensor->append_node(focus_distance);
				}
#if 0
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
					{
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
					// fov_axis
					{
						xml_node<> *fov_axis = doc.allocate_node(node_element, "string");
						xml_attribute<> *fov_axis_nameAttr = doc.allocate_attribute("name", "fov_axis");
						fov_axis->append_attribute(fov_axis_nameAttr);
						// fov_axis: x is default value, but we explicitly set to "x" (possibly, this could be updated later...)
						xml_attribute<> *fov_axis_valueAttr = doc.allocate_attribute("value", "x");
						fov_axis->append_attribute(fov_axis_valueAttr);
						sensor->append_node(fov_axis);
					}
				}
#else
				// fov
				{
					xml_node<> *fov = doc.allocate_node(node_element, "float");
					xml_attribute<> *fov_nameAttr = doc.allocate_attribute("name", "fov");
					fov->append_attribute(fov_nameAttr);
					const double distCameraToTarget = (center - cameraPosition).norm();
					double fovRad = std::asin((BSphereRadius * (1.0 + json.at("marginSize").get<float>())) / distCameraToTarget);
					char buf[255];
					snprintf(buf, 255, "%f", fovRad * 2.0 * 180.0 / M_PI);
					char *fov_value = doc.allocate_string(buf);
					xml_attribute<> *fov_valueAttr = doc.allocate_attribute("value", fov_value);
					fov->append_attribute(fov_valueAttr);
					sensor->append_node(fov);
				}
				// fov_axis
				{
					xml_node<> *fov_axis = doc.allocate_node(node_element, "string");
					xml_attribute<> *fov_axis_nameAttr = doc.allocate_attribute("name", "fov_axis");
					fov_axis->append_attribute(fov_axis_nameAttr);
					// fov_axis: x is default value, but we explicitly set to "x" (possibly, this could be updated later...)
					xml_attribute<> *fov_axis_valueAttr = doc.allocate_attribute("value", "smaller");
					fov_axis->append_attribute(fov_axis_valueAttr);
					sensor->append_node(fov_axis);
				}
#endif
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

				{
					// emitter 1: area light (1)
					// <shape type="sphere">
					// 	<emitter type="area">
					// 		<spectrum name="radiance" value="1.000000"/>
					// 	</emitter>
					// 	<transform name="to_world">
					// 		<translate x="500.000000" y="89.059668" z="402.218123"/>
					// 	</transform>
					// </shape>
					xml_node<> *shape = doc.allocate_node(node_element, "shape");
					xml_attribute<> *shapeAttr = doc.allocate_attribute("type", "sphere");
					shape->append_attribute(shapeAttr);
					{
						xml_node<> *radius = doc.allocate_node(node_element, "float");
						xml_attribute<> *radius_nameAttr = doc.allocate_attribute("name", "radius");
						radius->append_attribute(radius_nameAttr);
						xml_attribute<> *radius_valueAttr = doc.allocate_attribute("value", "100.0");
						radius->append_attribute(radius_valueAttr);
						shape->append_node(radius);
					}
					{
						xml_node<> *emitter = doc.allocate_node(node_element, "emitter");
						xml_attribute<> *emitterAttr = doc.allocate_attribute("type", "area");
						emitter->append_attribute(emitterAttr);
						{
							xml_node<> *spectrum = doc.allocate_node(node_element, "spectrum");
							xml_attribute<> *spectrumAttr = doc.allocate_attribute("name", "radiance");
							spectrum->append_attribute(spectrumAttr);
							char buf[255];
							snprintf(buf, 255, "%f", emitterJson.at("area").get<float>());
							char *spectrum_value = doc.allocate_string(buf);
							xml_attribute<> *spectrum_valueAttr = doc.allocate_attribute("value", spectrum_value);
							spectrum->append_attribute(spectrum_valueAttr);

							emitter->append_node(spectrum);
						}
						shape->append_node(emitter);
					}
					{
						// transform
						xml_node<> *transform = doc.allocate_node(node_element, "transform");
						xml_attribute<> *transformAttr = doc.allocate_attribute("name", "to_world");
						transform->append_attribute(transformAttr);
						{
							// mitsuba2 is Y-up coordinate
							xml_node<> *translate = doc.allocate_node(node_element, "translate");
							{
								char buf[255];
								// todo tweak sensor position based on camera angle in ZBrush
								snprintf(buf, 255, "%f", cameraPosition(0, 0) - 500.0f);
								char *x_value = doc.allocate_string(buf);
								xml_attribute<> *xAttr = doc.allocate_attribute("x", x_value);
								translate->append_attribute(xAttr);
							}
							{
								char buf[255];
								// todo tweak sensor position based on camera angle in ZBrush
								snprintf(buf, 255, "%f", cameraPosition(0, 1) + 250.0f);
								char *y_value = doc.allocate_string(buf);
								xml_attribute<> *yAttr = doc.allocate_attribute("y", y_value);
								translate->append_attribute(yAttr);
							}
							{
								char buf[255];
								// todo tweak sensor position based on camera angle in ZBrush
								snprintf(buf, 255, "%f", cameraPosition(0, 2));
								char *z_value = doc.allocate_string(buf);
								xml_attribute<> *zAttr = doc.allocate_attribute("z", z_value);
								translate->append_attribute(zAttr);
							}
							transform->append_node(translate);
						}
						shape->append_node(transform);
					}
					scene->append_node(shape);
				}
				{
					// emitter 2: area light (2)
					xml_node<> *shape = doc.allocate_node(node_element, "shape");
					xml_attribute<> *shapeAttr = doc.allocate_attribute("type", "sphere");
					shape->append_attribute(shapeAttr);
					{
						xml_node<> *radius = doc.allocate_node(node_element, "float");
						xml_attribute<> *radius_nameAttr = doc.allocate_attribute("name", "radius");
						radius->append_attribute(radius_nameAttr);
						xml_attribute<> *radius_valueAttr = doc.allocate_attribute("value", "100.0");
						radius->append_attribute(radius_valueAttr);
						shape->append_node(radius);
					}
					{
						xml_node<> *emitter = doc.allocate_node(node_element, "emitter");
						xml_attribute<> *emitterAttr = doc.allocate_attribute("type", "area");
						emitter->append_attribute(emitterAttr);
						{
							xml_node<> *spectrum = doc.allocate_node(node_element, "spectrum");
							xml_attribute<> *spectrumAttr = doc.allocate_attribute("name", "radiance");
							spectrum->append_attribute(spectrumAttr);
							char buf[255];
							snprintf(buf, 255, "%f", emitterJson.at("area").get<float>());
							char *spectrum_value = doc.allocate_string(buf);
							xml_attribute<> *spectrum_valueAttr = doc.allocate_attribute("value", spectrum_value);
							spectrum->append_attribute(spectrum_valueAttr);

							emitter->append_node(spectrum);
						}
						shape->append_node(emitter);
					}
					{
						// transform
						xml_node<> *transform = doc.allocate_node(node_element, "transform");
						xml_attribute<> *transformAttr = doc.allocate_attribute("name", "to_world");
						transform->append_attribute(transformAttr);
						{
							// mitsuba2 is Y-up coordinate
							xml_node<> *translate = doc.allocate_node(node_element, "translate");
							{
								char buf[255];
								// todo tweak sensor position based on camera angle in ZBrush
								snprintf(buf, 255, "%f", cameraPosition(0, 0) + 500.0f);
								char *x_value = doc.allocate_string(buf);
								xml_attribute<> *xAttr = doc.allocate_attribute("x", x_value);
								translate->append_attribute(xAttr);
							}
							{
								char buf[255];
								// todo tweak sensor position based on camera angle in ZBrush
								snprintf(buf, 255, "%f", cameraPosition(0, 1) + 50.0f);
								char *y_value = doc.allocate_string(buf);
								xml_attribute<> *yAttr = doc.allocate_attribute("y", y_value);
								translate->append_attribute(yAttr);
							}
							{
								char buf[255];
								// todo tweak sensor position based on camera angle in ZBrush
								snprintf(buf, 255, "%f", cameraPosition(0, 2));
								char *z_value = doc.allocate_string(buf);
								xml_attribute<> *zAttr = doc.allocate_attribute("z", z_value);
								translate->append_attribute(zAttr);
							}
							transform->append_node(translate);
						}
						shape->append_node(transform);
					}
					scene->append_node(shape);
				}
				{
					// emitter 3: area light (3)
					xml_node<> *shape = doc.allocate_node(node_element, "shape");
					xml_attribute<> *shapeAttr = doc.allocate_attribute("type", "sphere");
					shape->append_attribute(shapeAttr);
					{
						xml_node<> *radius = doc.allocate_node(node_element, "float");
						xml_attribute<> *radius_nameAttr = doc.allocate_attribute("name", "radius");
						radius->append_attribute(radius_nameAttr);
						xml_attribute<> *radius_valueAttr = doc.allocate_attribute("value", "100.0");
						radius->append_attribute(radius_valueAttr);
						shape->append_node(radius);
					}
					{
						xml_node<> *emitter = doc.allocate_node(node_element, "emitter");
						xml_attribute<> *emitterAttr = doc.allocate_attribute("type", "area");
						emitter->append_attribute(emitterAttr);
						{
							xml_node<> *spectrum = doc.allocate_node(node_element, "spectrum");
							xml_attribute<> *spectrumAttr = doc.allocate_attribute("name", "radiance");
							spectrum->append_attribute(spectrumAttr);
							char buf[255];
							snprintf(buf, 255, "%f", emitterJson.at("area").get<float>());
							char *spectrum_value = doc.allocate_string(buf);
							xml_attribute<> *spectrum_valueAttr = doc.allocate_attribute("value", spectrum_value);
							spectrum->append_attribute(spectrum_valueAttr);

							emitter->append_node(spectrum);
						}
						shape->append_node(emitter);
					}
					{
						// transform
						xml_node<> *transform = doc.allocate_node(node_element, "transform");
						xml_attribute<> *transformAttr = doc.allocate_attribute("name", "to_world");
						transform->append_attribute(transformAttr);
						{
							// mitsuba2 is Y-up coordinate
							xml_node<> *translate = doc.allocate_node(node_element, "translate");
							{
								char buf[255];
								// todo tweak sensor position based on camera angle in ZBrush
								snprintf(buf, 255, "%f", cameraPosition(0, 0) + 100.0f);
								char *x_value = doc.allocate_string(buf);
								xml_attribute<> *xAttr = doc.allocate_attribute("x", x_value);
								translate->append_attribute(xAttr);
							}
							{
								char buf[255];
								// todo tweak sensor position based on camera angle in ZBrush
								snprintf(buf, 255, "%f", cameraPosition(0, 1) + 00.0f);
								char *y_value = doc.allocate_string(buf);
								xml_attribute<> *yAttr = doc.allocate_attribute("y", y_value);
								translate->append_attribute(yAttr);
							}
							{
								char buf[255];
								// todo tweak sensor position based on camera angle in ZBrush
								snprintf(buf, 255, "%f", cameraPosition(0, 2) + 150.0f);
								char *z_value = doc.allocate_string(buf);
								xml_attribute<> *zAttr = doc.allocate_attribute("z", z_value);
								translate->append_attribute(zAttr);
							}
							transform->append_node(translate);
						}
						shape->append_node(transform);
					}
					scene->append_node(shape);
				}
			}
			{
				// floor/back
				// target shape
				xml_node<> *shape = doc.allocate_node(node_element, "shape");
				xml_attribute<> *shapeAttr = doc.allocate_attribute("type", "ply");
				shape->append_attribute(shapeAttr);
				{
					xml_node<> *filename = doc.allocate_node(node_element, "string");
					xml_attribute<> *filenameAttr = doc.allocate_attribute("name", "filename");
					filename->append_attribute(filenameAttr);
					xml_attribute<> *filename_valueAttr = doc.allocate_attribute("value", "base.ply");
					filename->append_attribute(filename_valueAttr);

					shape->append_node(filename);
				}
				{
					xml_node<> *face_normals = doc.allocate_node(node_element, "boolean");
					xml_attribute<> *face_normalsAttr = doc.allocate_attribute("name", "face_normals");
					face_normals->append_attribute(face_normalsAttr);
					xml_attribute<> *face_normals_valueAttr = doc.allocate_attribute("value", "true");
					face_normals->append_attribute(face_normals_valueAttr);

					shape->append_node(face_normals);
				}
				{
					// bsdf
					xml_node<> *bsdf = doc.allocate_node(node_element, "bsdf");
					xml_attribute<> *bsdfAttr = doc.allocate_attribute("type", "diffuse");
					bsdf->append_attribute(bsdfAttr);
					{
						xml_node<> *rgb = doc.allocate_node(node_element, "rgb");
						xml_attribute<> *rgbAttr = doc.allocate_attribute("name", "reflectance");
						rgb->append_attribute(rgbAttr);
						char buf[255];
						snprintf(buf, 255, "%f, %f, %f", (json.at("background").at("r").get<float>() / 255.0f), (json.at("background").at("g").get<float>() / 255.0f), (json.at("background").at("b").get<float>() / 255.0f));
						char *rgb_value = doc.allocate_string(buf);
						xml_attribute<> *rgb_valueAttr = doc.allocate_attribute("value", rgb_value);
						rgb->append_attribute(rgb_valueAttr);

						bsdf->append_node(rgb);
					}
					shape->append_node(bsdf);
				}
				scene->append_node(shape);
			}
			{
				// back
				//   if camera views from below
				// TODO!!
			} {
				// target shape
				xml_node<> *shape = doc.allocate_node(node_element, "shape");
				xml_attribute<> *shapeAttr = doc.allocate_attribute("type", "ply");
				shape->append_attribute(shapeAttr);
				{
					xml_node<> *filename = doc.allocate_node(node_element, "string");
					xml_attribute<> *filenameAttr = doc.allocate_attribute("name", "filename");
					filename->append_attribute(filenameAttr);
					xml_attribute<> *filename_valueAttr = doc.allocate_attribute("value", "mesh.ply");
					filename->append_attribute(filename_valueAttr);

					shape->append_node(filename);
				}
				{
					// bsdf
					xml_node<> *bsdf = doc.allocate_node(node_element, "bsdf");
					xml_attribute<> *bsdfAttr = doc.allocate_attribute("type", "roughplastic");
					bsdf->append_attribute(bsdfAttr);
					if (hasVC)
					{
						xml_node<> *diffuse_reflectance = doc.allocate_node(node_element, "texture");
						xml_attribute<> *diffuse_reflectanceTypeAttr = doc.allocate_attribute("type", "mesh_attribute");
						diffuse_reflectance->append_attribute(diffuse_reflectanceTypeAttr);
						xml_attribute<> *diffuse_reflectanceNameAttr = doc.allocate_attribute("name", "diffuse_reflectance");
						diffuse_reflectance->append_attribute(diffuse_reflectanceNameAttr);
						{
							xml_node<> *vertex_color = doc.allocate_node(node_element, "string");
							xml_attribute<> *vertex_colorNameAttr = doc.allocate_attribute("name", "name");
							vertex_color->append_attribute(vertex_colorNameAttr);
							xml_attribute<> *vertex_colorValueAttr = doc.allocate_attribute("value", "vertex_color");
							vertex_color->append_attribute(vertex_colorValueAttr);

							diffuse_reflectance->append_node(vertex_color);
						}

						bsdf->append_node(diffuse_reflectance);
					}
					else
					{
						xml_node<> *diffuse_reflectance = doc.allocate_node(node_element, "spectrum");
						xml_attribute<> *diffuse_reflectanceAttr = doc.allocate_attribute("name", "diffuse_reflectance");
						diffuse_reflectance->append_attribute(diffuse_reflectanceAttr);
						xml_attribute<> *diffuse_reflectance_valueAttr = doc.allocate_attribute("value", "0.5");
						diffuse_reflectance->append_attribute(diffuse_reflectance_valueAttr);

						bsdf->append_node(diffuse_reflectance);
					}

					{
						// int_ior
						// PVC, 1.52 -- 1.55
						xml_node<> *int_ior = doc.allocate_node(node_element, "float");
						xml_attribute<> *int_iorAttr = doc.allocate_attribute("name", "int_ior");
						int_ior->append_attribute(int_iorAttr);
						xml_attribute<> *int_ior_valueAttr = doc.allocate_attribute("value", "1.535");
						int_ior->append_attribute(int_ior_valueAttr);

						bsdf->append_node(int_ior);
					}
					{
						// ext_ior
						// Air, 1.000277
						xml_node<> *ext_ior = doc.allocate_node(node_element, "float");
						xml_attribute<> *ext_iorAttr = doc.allocate_attribute("name", "ext_ior");
						ext_ior->append_attribute(ext_iorAttr);
						xml_attribute<> *ext_ior_valueAttr = doc.allocate_attribute("value", "1.000277");
						ext_ior->append_attribute(ext_ior_valueAttr);

						bsdf->append_node(ext_ior);
					}
					{
						// distribution
						xml_node<> *distribution = doc.allocate_node(node_element, "string");
						xml_attribute<> *distributionAttr = doc.allocate_attribute("name", "distribution");
						distribution->append_attribute(distributionAttr);
						xml_attribute<> *distribution_valueAttr = doc.allocate_attribute("value", "beckmann");
						distribution->append_attribute(distribution_valueAttr);

						bsdf->append_node(distribution);
					}
					{
						// alpha (roughness)
						xml_node<> *alpha = doc.allocate_node(node_element, "float");
						xml_attribute<> *alphaAttr = doc.allocate_attribute("name", "alpha");
						alpha->append_attribute(alphaAttr);
						xml_attribute<> *alpha_valueAttr = doc.allocate_attribute("value", "0.3");
						alpha->append_attribute(alpha_valueAttr);

						bsdf->append_node(alpha);
					}

					shape->append_node(bsdf);
				}
				scene->append_node(shape);
			}
			doc.append_node(scene);
		}
		std::cout << doc << std::endl;

		// write to file
		std::ofstream xml(xmlPath);
		const std::string declaration("<?xml version=\"1.0\"?>");
		xml << declaration;
		xml << doc;
	}

	////
	// call mitsuba executable
	{
		fs::path mitsubaExePath(rootPath);
		mitsubaExePath.append("resources");
		mitsubaExePath.append("mitsuba2");
		mitsubaExePath.append("mitsuba.exe");

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

			fs::path imagePath(dataPath);
			std::string imageFileName(exrPath.stem().string());
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
