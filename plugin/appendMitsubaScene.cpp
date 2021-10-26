#ifndef APPEND_MITSUBA_SCENE_CPP
#define APPEND_MITSUBA_SCENE_CPP

#include "appendMitsubaScene.h"
#include "appendMitsubaIntegrator.h"
#include "appendMitsubaSensor.h"
#include "appendMitsubaEmitter.h"
#include "appendMitsubaBaseShape.h"
#include "appendMitsubaTargetShape.h"

void appendMitsubaScene(xml_document<> &doc, nlohmann::json &parameters)
{
	xml_node<> *scene = doc.allocate_node(node_element, "scene");
	xml_attribute<> *sceneAttr = doc.allocate_attribute("version", "2.0.0");
	scene->append_attribute(sceneAttr);
	// integrator
	appendMitsubaIntegrator(doc, scene);
	// sensor
	appendMitsubaSensor(doc, scene, parameters.at("sensor"));
	// emitters
	// we copy parameters for calculate emitter positions
	parameters.at("emitters")["origin"] = parameters.at("sensor").at("position");
	parameters.at("emitters")["viewRight"] = parameters.at("sensor").at("viewRight");
	parameters.at("emitters")["viewUp"] = parameters.at("sensor").at("viewUp");
	parameters.at("emitters")["viewDir"] = parameters.at("sensor").at("viewDir");
	appendMitsubaEmitter(doc, scene, parameters.at("emitters"));
	// shapes
	parameters.at("shapes")["viewDir"] = parameters.at("sensor").at("viewDir");
	appendMitsubaBaseShape(doc, scene, parameters.at("shapes"));
	appendMitsubaTargetShape(doc, scene, parameters.at("shapes"));

	doc.append_node(scene);
}

#endif
