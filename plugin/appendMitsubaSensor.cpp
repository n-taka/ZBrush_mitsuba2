#ifndef APPEND_MITSUBA_SENSOR_CPP
#define APPEND_MITSUBA_SENSOR_CPP

#include "appendMitsubaSensor.h"
#include "appendMitsubaFilm.h"
#include "appendMitsubaSampler.h"
#include "xmlAppendAttribute.h"

#include "Eigen/Core"

void appendMitsubaSensor(xml_document<> &doc, xml_node<> *scene, nlohmann::json &parameters)
{
	xml_node<> *sensor = doc.allocate_node(node_element, "sensor");
	xmlAppendAttribute(doc, sensor, "type", "thinlens");

	// transform
	xml_node<> *transform = doc.allocate_node(node_element, "transform");
	xmlAppendAttribute(doc, transform, "name", "to_world");

	Eigen::Matrix<float, 1, Eigen::Dynamic> cameraPosition;
	Eigen::Matrix<float, 1, Eigen::Dynamic> BSphereCenter;
	BSphereCenter.resize(1, 3);
	BSphereCenter(0, 0) = parameters.at("BSphere").at("center").at("x").get<float>();
	BSphereCenter(0, 1) = parameters.at("BSphere").at("center").at("y").get<float>();
	BSphereCenter(0, 2) = parameters.at("BSphere").at("center").at("z").get<float>();
	const float BSphereRadius = parameters.at("BSphere").at("radius").get<float>();
	{
		cameraPosition = BSphereCenter;
		Eigen::Matrix<float, 1, Eigen::Dynamic> unitVectorFromTargetToCamera;
		unitVectorFromTargetToCamera.resize(1, 3);
		unitVectorFromTargetToCamera(0, 0) = parameters.at("viewDir").at("x").get<float>();
		unitVectorFromTargetToCamera(0, 1) = parameters.at("viewDir").at("y").get<float>();
		unitVectorFromTargetToCamera(0, 2) = parameters.at("viewDir").at("z").get<float>();
		unitVectorFromTargetToCamera *= -1;
		unitVectorFromTargetToCamera.normalize();

		const float BSphereRadiusWithMargin = (BSphereRadius * (1.0 + parameters.at("marginSize").get<float>()));
		const float dist = BSphereRadiusWithMargin / std::sin(0.5 * parameters.at("fov").get<float>() * M_PI / 180.0);
		cameraPosition += dist * (unitVectorFromTargetToCamera);
		parameters["position"] = nlohmann::json::object();
		parameters.at("position")["x"] = cameraPosition(0, 0);
		parameters.at("position")["y"] = cameraPosition(0, 1);
		parameters.at("position")["z"] = cameraPosition(0, 2);
		xml_node<> *lookat = doc.allocate_node(node_element, "lookat");
		std::stringstream ss;
		ss << BSphereCenter(0, 0) << "," << BSphereCenter(0, 1) << "," << BSphereCenter(0, 2);
		xmlAppendAttribute(doc, lookat, "target", ss.str());
		ss.str("");
		ss << cameraPosition(0, 0) << "," << cameraPosition(0, 1) << "," << cameraPosition(0, 2);
		xmlAppendAttribute(doc, lookat, "origin", ss.str());
		ss.str("");
		Eigen::Matrix<float, 1, Eigen::Dynamic> cameraUp;
		cameraUp.resize(1, 3);
		cameraUp(0, 0) = parameters.at("viewUp").at("x").get<float>();
		cameraUp(0, 1) = parameters.at("viewUp").at("y").get<float>();
		cameraUp(0, 2) = parameters.at("viewUp").at("z").get<float>();
		cameraUp.normalize();
		ss << cameraUp(0, 0) << "," << cameraUp(0, 1) << "," << cameraUp(0, 2);
		xmlAppendAttribute(doc, lookat, "up", ss.str());
		transform->append_node(lookat);
	}
	sensor->append_node(transform);
	// aperture_radius
	xml_node<> *aperture_radius = doc.allocate_node(node_element, "float");
	xmlAppendAttribute(doc, aperture_radius, "name", "aperture_radius");
	xmlAppendAttribute(doc, aperture_radius, "value", parameters.at("aperture_radius").get<float>());
	sensor->append_node(aperture_radius);
	// focus_distance
	xml_node<> *focus_distance = doc.allocate_node(node_element, "float");
	xmlAppendAttribute(doc, focus_distance, "name", "focus_distance");
	float focusDist = (BSphereCenter - cameraPosition).norm();
	focusDist += (parameters.at("focus_distance").get<float>() * BSphereRadius);
	xmlAppendAttribute(doc, focus_distance, "value", focusDist);
	sensor->append_node(focus_distance);
	// fov
	xml_node<> *fov = doc.allocate_node(node_element, "float");
	xmlAppendAttribute(doc, fov, "name", "fov");
	xmlAppendAttribute(doc, fov, "value", parameters.at("fov").get<float>());
	sensor->append_node(fov);
	// fov_axis
	xml_node<> *fov_axis = doc.allocate_node(node_element, "string");
	xmlAppendAttribute(doc, fov_axis, "name", "fov_axis");
	xmlAppendAttribute(doc, fov_axis, "value", "smaller");
	sensor->append_node(fov_axis);
	// near_clip
	xml_node<> *near_clip = doc.allocate_node(node_element, "float");
	xmlAppendAttribute(doc, near_clip, "name", "near_clip");
	xmlAppendAttribute(doc, near_clip, "value", parameters.at("near_clip").get<float>());
	sensor->append_node(near_clip);
	// far_clip
	xml_node<> *far_clip = doc.allocate_node(node_element, "float");
	xmlAppendAttribute(doc, far_clip, "name", "far_clip");
	xmlAppendAttribute(doc, far_clip, "value", parameters.at("far_clip").get<float>());
	sensor->append_node(far_clip);

	// sampler
	const nlohmann::json &samplerJson = parameters.at("sampler");
	appendMitsubaSampler(doc, sensor, samplerJson);
	// film
	const nlohmann::json &filmJson = parameters.at("film");
	appendMitsubaFilm(doc, sensor, filmJson);

	scene->append_node(sensor);
}

#endif
