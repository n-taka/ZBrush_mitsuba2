#ifndef APPEND_MITSUBA_FILM_CPP
#define APPEND_MITSUBA_FILM_CPP

#include "appendMitsubaFilm.h"
#include "xmlAppendAttribute.h"

void appendMitsubaFilm(xml_document<> &doc, xml_node<> *sensor, const nlohmann::json &parameters)
{
	xml_node<> *film = doc.allocate_node(node_element, "film");
	xmlAppendAttribute(doc, film, "type", "hdrfilm");
	// width
	xml_node<> *width = doc.allocate_node(node_element, "integer");
	xmlAppendAttribute(doc, width, "name", "width");
	xmlAppendAttribute(doc, width, "value", parameters.at("width").get<int>());
	film->append_node(width);
	// height
	xml_node<> *height = doc.allocate_node(node_element, "integer");
	xmlAppendAttribute(doc, height, "name", "height");
	xmlAppendAttribute(doc, height, "value", parameters.at("height").get<int>());
	film->append_node(height);
	// rfilter
	xml_node<> *rfilter = doc.allocate_node(node_element, "rfilter");
	xmlAppendAttribute(doc, rfilter, "type", parameters.at("rfilter").get<std::string>());
	film->append_node(rfilter);

	sensor->append_node(film);
}

#endif
