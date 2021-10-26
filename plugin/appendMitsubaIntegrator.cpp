#ifndef APPEND_MITSUBA_INTEGRATOR_CPP
#define APPEND_MITSUBA_INTEGRATOR_CPP

#include "appendMitsubaIntegrator.h"
#include "xmlAppendAttribute.h"

void appendMitsubaIntegrator(xml_document<> &doc, xml_node<> *scene)
{
	// currently, we only support path tracer
	xml_node<> *integrator = doc.allocate_node(node_element, "integrator");
	xmlAppendAttribute(doc, integrator, "type", "path");
	
	scene->append_node(integrator);
}

#endif
