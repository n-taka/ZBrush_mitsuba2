#ifndef APPEND_MITSUBA_TARGET_SHAPE_CPP
#define APPEND_MITSUBA_TARGET_SHAPE_CPP

#include "appendMitsubaTargetShape.h"
#include "xmlAppendAttribute.h"

#include <sstream>
#include "Eigen/Core"

void appendMitsubaTargetShape(xml_document<> &doc, xml_node<> *scene, const nlohmann::json &parameters)
{
	// target shape
	xml_node<> *shape = doc.allocate_node(node_element, "shape");
	xmlAppendAttribute(doc, shape, "type", "ply");
	// shape/filename
	xml_node<> *filename = doc.allocate_node(node_element, "string");
	xmlAppendAttribute(doc, filename, "name", "filename");
	xmlAppendAttribute(doc, filename, "value", "mesh.ply");
	shape->append_node(filename);
	xml_node<> *face_normals = doc.allocate_node(node_element, "boolean");
	xmlAppendAttribute(doc, face_normals, "name", "face_normals");
	xmlAppendAttribute(doc, face_normals, "value", "true");
	shape->append_node(face_normals);
	// shape/bsdf
	xml_node<> *bsdf = doc.allocate_node(node_element, "bsdf");
	xmlAppendAttribute(doc, bsdf, "type", "roughplastic");
	if (parameters.at("hasVC").get<bool>())
	{
		// shape/bsdf/texture
		xml_node<> *diffuse_reflectance = doc.allocate_node(node_element, "texture");
		xmlAppendAttribute(doc, diffuse_reflectance, "type", "mesh_attribute");
		xmlAppendAttribute(doc, diffuse_reflectance, "name", "diffuse_reflectance");
		// shape/bsdf/texture/string
		xml_node<> *vertex_color = doc.allocate_node(node_element, "string");
		xmlAppendAttribute(doc, vertex_color, "name", "name");
		xmlAppendAttribute(doc, vertex_color, "value", "vertex_color");
		diffuse_reflectance->append_node(vertex_color);
		bsdf->append_node(diffuse_reflectance);
	}
	else
	{
		// no vertex color => gray
		xml_node<> *diffuse_reflectance = doc.allocate_node(node_element, "spectrum");
		xmlAppendAttribute(doc, diffuse_reflectance, "name", "diffuse_reflectance");
		xmlAppendAttribute(doc, diffuse_reflectance, "value", "0.5");
		bsdf->append_node(diffuse_reflectance);
	}

	// shape/bsdf/int_ior
	// PVC, 1.52 -- 1.55
	xml_node<> *int_ior = doc.allocate_node(node_element, "float");
	xmlAppendAttribute(doc, int_ior, "name", "int_ior");
	xmlAppendAttribute(doc, int_ior, "value", 1.535);
	bsdf->append_node(int_ior);

	// shape/bsdf/ext_ior
	// Air, 1.000277
	xml_node<> *ext_ior = doc.allocate_node(node_element, "float");
	xmlAppendAttribute(doc, ext_ior, "name", "ext_ior");
	xmlAppendAttribute(doc, ext_ior, "value", 1.000277);
	bsdf->append_node(ext_ior);

	// shape/bsdf/distribution
	xml_node<> *distribution = doc.allocate_node(node_element, "string");
	xmlAppendAttribute(doc, distribution, "name", "distribution");
	xmlAppendAttribute(doc, distribution, "value", "beckmann");
	bsdf->append_node(distribution);

	// shape/bsdf/alpha (roughness)
	xml_node<> *alpha = doc.allocate_node(node_element, "float");
	xmlAppendAttribute(doc, alpha, "name", "alpha");
	xmlAppendAttribute(doc, alpha, "value", 0.3);
	bsdf->append_node(alpha);

	shape->append_node(bsdf);
	scene->append_node(shape);
}

#endif
