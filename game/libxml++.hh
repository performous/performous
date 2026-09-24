#pragma once

#include "config.hh"
#include "util.hh"

#include <libxml++/libxml++.h>

#include <vector>

// A few forward definitions of libxml++ types
namespace xmlpp {

#if LIBXMLPP_VERSION_2_6
	using strType = Glib::ustring;
	using const_NodeSet = NodeSet;
#elif LIBXMLPP_VERSION_3_0
	using strType = Glib::ustring;
	using const_NodeSet = Node::const_NodeSet;
#elif LIBXMLPP_VERSION_5_0
	using strType = std::string;
	using const_NodeSet = Node::const_NodeSet;
#endif

	class Node;
	class Element;
	typedef std::vector<Node*> NodeSet;

	struct XMLError {
		XMLError(xmlpp::Element const& e, std::string const& msg): elem(e), message(msg) {}
		xmlpp::Element const& elem;
		std::string message;
	};

	std::string getAttribute(xmlpp::Element const& elem, std::string const& attr);
	Element* add_child_element(Element* element, const strType& name);
	const TextNode* get_first_child_text(const Element& element);
	void set_first_child_text(Element* element, const strType& content);

	template <typename T> bool tryGetAttribute(xmlpp::Element const& elem, std::string const& attr, T& var) {
		xmlpp::Attribute const* a = elem.get_attribute(attr);
		if (!a) return false;
		try {
			var = sconv<T>(a->get_value());
		} catch (std::exception&) {
			throw XMLError(elem, "attribute " + attr + " value invalid: " + a->get_value());
		}
		return true;
	}
	template <typename Numeric> void parse(MinMax<Numeric>& range, xmlpp::Element const& elem) {
		tryGetAttribute(elem, "min", range.min);
		tryGetAttribute(elem, "max", range.max);
	}
}

