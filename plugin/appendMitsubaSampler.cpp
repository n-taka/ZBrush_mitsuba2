#ifndef APPEND_MITSUBA_SAMPLER_CPP
#define APPEND_MITSUBA_SAMPLER_CPP

#include "appendMitsubaSampler.h"
#include "xmlAppendAttribute.h"

void appendMitsubaSampler(xml_document<> &doc, xml_node<> *sensor, const nlohmann::json &parameters)
{
	xml_node<> *sampler = doc.allocate_node(node_element, "sampler");
	xmlAppendAttribute(doc, sampler, "type", parameters.at("type").get<std::string>());

	// sample_count
	xml_node<> *sample_count = doc.allocate_node(node_element, "integer");
	xmlAppendAttribute(doc, sample_count, "name", "sample_count");
	xmlAppendAttribute(doc, sample_count, "value", parameters.at("sample_count").get<int>());
	sampler->append_node(sample_count);

	sensor->append_node(sampler);
}

#endif
