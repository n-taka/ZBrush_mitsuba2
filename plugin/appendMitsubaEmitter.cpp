#ifndef APPEND_MITSUBA_EMITTER_CPP
#define APPEND_MITSUBA_EMITTER_CPP

#include "appendMitsubaEmitter.h"
#include "xmlAppendAttribute.h"

#include "Eigen/Core"

void appendMitsubaEmitter(xml_document<> &doc, xml_node<> *scene, const nlohmann::json &parameters)
{
	// emitter 0: constant
	xml_node<> *constant = doc.allocate_node(node_element, "emitter");
	xmlAppendAttribute(doc, constant, "type", "constant");
	// constant-spectrum
	xml_node<> *spectrum = doc.allocate_node(node_element, "spectrum");
	xmlAppendAttribute(doc, spectrum, "name", "radiance");
	xmlAppendAttribute(doc, spectrum, "value", parameters.at("constant").get<float>());
	constant->append_node(spectrum);
	scene->append_node(constant);

	Eigen::Matrix<float, 1, Eigen::Dynamic> origin;
	origin.resize(1, 3);
	origin(0, 0) = parameters.at("origin").at("x").get<float>();
	origin(0, 1) = parameters.at("origin").at("y").get<float>();
	origin(0, 2) = parameters.at("origin").at("z").get<float>();
	Eigen::Matrix<float, 1, Eigen::Dynamic> up;
	up.resize(1, 3);
	up(0, 0) = parameters.at("viewUp").at("x").get<float>();
	up(0, 1) = parameters.at("viewUp").at("y").get<float>();
	up(0, 2) = parameters.at("viewUp").at("z").get<float>();
	up.normalize();
	Eigen::Matrix<float, 1, Eigen::Dynamic> right;
	right.resize(1, 3);
	right(0, 0) = parameters.at("viewRight").at("x").get<float>();
	right(0, 1) = parameters.at("viewRight").at("y").get<float>();
	right(0, 2) = parameters.at("viewRight").at("z").get<float>();
	right.normalize();
	Eigen::Matrix<float, 1, Eigen::Dynamic> dir;
	dir.resize(1, 3);
	dir(0, 0) = parameters.at("viewDir").at("x").get<float>();
	dir(0, 1) = parameters.at("viewDir").at("y").get<float>();
	dir(0, 2) = parameters.at("viewDir").at("z").get<float>();
	dir.normalize();

	std::vector<Eigen::Matrix<float, 1, Eigen::Dynamic>> positions;
	positions.resize(3);
	positions.at(0) = origin + (-100 * right) + (50 * up) + (0 * dir);
	positions.at(1) = origin + (100 * right) + (10 * up) + (0 * dir);
	positions.at(2) = origin + (20 * right) + (0 * up) + (-30 * dir);

	for (const auto &pos : positions)
	{
		// emitter 1: area light (1)
		xml_node<> *shape = doc.allocate_node(node_element, "shape");
		xmlAppendAttribute(doc, shape, "type", "sphere");
		// shape/radius
		xml_node<> *radius = doc.allocate_node(node_element, "float");
		xmlAppendAttribute(doc, radius, "name", "radius");
		xmlAppendAttribute(doc, radius, "value", 10.0);
		shape->append_node(radius);
		// shape/emitter
		xml_node<> *emitter = doc.allocate_node(node_element, "emitter");
		xmlAppendAttribute(doc, emitter, "type", "area");
		// shape/emitter/spectrum
		xml_node<> *spectrum = doc.allocate_node(node_element, "spectrum");
		xmlAppendAttribute(doc, spectrum, "name", "radiance");
		xmlAppendAttribute(doc, spectrum, "value", parameters.at("area").get<float>());
		emitter->append_node(spectrum);
		shape->append_node(emitter);
		// shape/transform
		xml_node<> *transform = doc.allocate_node(node_element, "transform");
		xmlAppendAttribute(doc, transform, "name", "to_world");
		xml_node<> *translate = doc.allocate_node(node_element, "translate");
		xmlAppendAttribute(doc, translate, "x", pos(0, 0));
		xmlAppendAttribute(doc, translate, "y", pos(0, 1));
		xmlAppendAttribute(doc, translate, "z", pos(0, 2));
		transform->append_node(translate);
		shape->append_node(transform);
		scene->append_node(shape);
	}
}

#endif
