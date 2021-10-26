#ifndef APPEND_MITSUBA_BASE_SHAPE_CPP
#define APPEND_MITSUBA_BASE_SHAPE_CPP

#include "appendMitsubaBaseShape.h"
#include "xmlAppendAttribute.h"

#include <sstream>
#include "Eigen/Core"

void appendMitsubaBaseShape(xml_document<> &doc, xml_node<> *scene, const nlohmann::json &parameters)
{
	Eigen::Matrix<float, 1, Eigen::Dynamic> dir;
	dir.resize(1, 3);
	dir(0, 0) = parameters.at("viewDir").at("x").get<float>();
	dir(0, 1) = parameters.at("viewDir").at("y").get<float>();
	dir(0, 2) = parameters.at("viewDir").at("z").get<float>();
	dir.normalize();

	const float viewAngle = std::asin(dir(0, 2)) * 180.0f / M_PI;

	// if view downward (here, I use 5 deg.)
	xml_node<> *shape = doc.allocate_node(node_element, "shape");
	xmlAppendAttribute(doc, shape, "type", "ply");
	// shape/filename
	xml_node<> *filename = doc.allocate_node(node_element, "string");
	xmlAppendAttribute(doc, filename, "name", "filename");
	xmlAppendAttribute(doc, filename, "value", (viewAngle < 5.0f ? "../resources/baseAndBackGround.ply" : "../resources/backGround.ply"));
	shape->append_node(filename);
	// shape/face_normals
	xml_node<> *face_normals = doc.allocate_node(node_element, "boolean");
	xmlAppendAttribute(doc, face_normals, "name", "face_normals");
	xmlAppendAttribute(doc, face_normals, "value", "true");
	shape->append_node(face_normals);
	// shape/bsdf
	xml_node<> *bsdf = doc.allocate_node(node_element, "bsdf");
	xmlAppendAttribute(doc, bsdf, "type", "diffuse");
	xml_node<> *rgb = doc.allocate_node(node_element, "rgb");
	xmlAppendAttribute(doc, rgb, "name", "reflectance");
	std::stringstream ss;
	ss << parameters.at("background").at("r").get<float>() / 255.0f << "," << parameters.at("background").at("g").get<float>() / 255.0f << "," << parameters.at("background").at("b").get<float>() / 255.0f;
	xmlAppendAttribute(doc, rgb, "value", ss.str());
	bsdf->append_node(rgb);
	shape->append_node(bsdf);
	scene->append_node(shape);
}

#endif
