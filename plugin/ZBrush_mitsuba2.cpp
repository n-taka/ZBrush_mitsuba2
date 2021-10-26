#ifndef ZBRUSH_MITSUBA2_CPP
#define ZBRUSH_MITSUBA2_CPP

#include "ZBrush_mitsuba2.h"
#include "nlohmann/json.hpp"
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

#include "readGoZAndTriangulate.h"
#include "getMinimumBoundingSphere.h"

#include "appendMitsubaScene.h"
#include "exportImage.h"

#include <string>
#include <sstream>
#include <fstream>

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
	// parse parameter (JSON)
	std::string jsonString(someText);
	nlohmann::json json = nlohmann::json::parse(jsonString);
	const std::string &rootString = json.at("root").get<std::string>();
	const fs::path rootPath(rootString);
	const std::string timeStamp = getCurrentTimestampAsString();

	std::cout << json.dump(4) << std::endl;

	////
	// calculate rotation of the mesh
	//   Here we load GoZ file format (because such data contains complete information)
	//   Then we export rotated mesh in ply format
	const std::string &meshFileRelPathStr = json.at("meshFile").get<std::string>();
	fs::path meshFileRelPath(meshFileRelPathStr);
	fs::path meshFilePath(rootPath);
	meshFilePath /= meshFileRelPath;
	const float BSphereRadius = 50.0f;
	Eigen::Matrix<float, 1, Eigen::Dynamic> center;
	bool hasVC = false;
	{

		std::string meshName;
		Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> V;
		Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> F;
		Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> VC;
		readGoZAndTriangulate(meshFilePath, meshName, V, F, VC);
		json["meshName"] = meshName;

		//   In our setting, we scale so that its minimum bounding sphere has radius of 50.
		//   * We use sphere so that sphere is rotational symmetry
		float radius;
		getMinimumBoundingSphere(V, F, center, radius);
		// scale
		V *= (BSphereRadius / radius);
		// translate
		//   center of the bounding sphere is aligned on the Y axis
		//   the lowest position of the mesh is aligned on the ground (y=0)
		Eigen::Matrix<float, 1, Eigen::Dynamic> translation;
		translation.resize(1, 3);
		translation(0, 0) = -center(0, 0);
		translation(0, 1) = -V.col(1).minCoeff();
		translation(0, 2) = -center(0, 2);
		V.rowwise() += translation;
		center += translation;
		json.at("mitsuba").at("sensor")["BSphere"] = nlohmann::json::object();
		json.at("mitsuba").at("sensor").at("BSphere")["radius"] = BSphereRadius;
		json.at("mitsuba").at("sensor").at("BSphere")["center"] = nlohmann::json::object();
		json.at("mitsuba").at("sensor").at("BSphere").at("center")["x"] = center(0, 0);
		json.at("mitsuba").at("sensor").at("BSphere").at("center")["y"] = center(0, 1);
		json.at("mitsuba").at("sensor").at("BSphere").at("center")["z"] = center(0, 2);

		// export to ply file to the same directory with GoZ file
		fs::path meshPath(meshFilePath);
		meshPath = meshPath.parent_path();
		meshPath.append("mesh.ply");
		// todo tweak for vertex colors
		Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> N;
		igl::per_vertex_normals(V, F, N);
		Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> UV;
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
		json.at("mitsuba").at("shapes")["hasVC"] = hasVC;
	}

	////
	// setup xml
	fs::path xmlPath(meshFilePath);
	xmlPath = xmlPath.parent_path();
	fs::path exrPath(xmlPath);
	{
		std::string xmlFileName(json.at("meshName").get<std::string>());
		xmlFileName += "_";
		xmlFileName += timeStamp;
		xmlFileName += ".xml";
		xmlPath.append(xmlFileName);
		std::string exrFileName(json.at("meshName").get<std::string>());
		exrFileName += "_";
		exrFileName += timeStamp;
		exrFileName += ".exr";
		exrPath.append(exrFileName);

		rapidxml::xml_document<> doc;
		appendMitsubaScene(doc, json.at("mitsuba"));

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

	exportImage(exrPath, json.at("export"));

	return 0.0;
}

#endif
