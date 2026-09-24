#include "libxml++.hh"

namespace xmlpp {
	Element* add_child_element(Element* element, const strType& name) {
		#if LIBXMLPP_VERSION_2_6
			return element->add_child(name);
		#else
			return element->add_child_element(name);
		#endif
	}

	const TextNode* get_first_child_text(const Element& element) {
		#if LIBXMLPP_VERSION_2_6
			return element.get_child_text();
		#else
			return element.get_first_child_text();
		#endif
	}

	void set_first_child_text(Element* element, const strType& content) {
		#if LIBXMLPP_VERSION_2_6
			return element->set_child_text(content);
		#else
			return element->set_first_child_text(content);
		#endif
	}

	std::string getAttribute(xmlpp::Element const& elem, std::string const& attr) {
		xmlpp::Attribute const* a = elem.get_attribute(attr);
		if (!a) throw XMLError(elem, "attribute " + attr + " not found");
		return a->get_value();
	}
}