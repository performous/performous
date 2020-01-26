#pragma once

#include "config.hh"

#include <libxml++/libxml++.h>

namespace xmlpp {
#if LIBXMLPP_VERSION == 2
	typedef NodeSet const_NodeSet; // implementation to satisfy libxml++ 2.6 API

	static inline Element* add_child_element(Element* element, const Glib::ustring& name) {
		return element->add_child(name);
	}

	static inline const TextNode* get_first_child_text(const Element& element) {
		return element.get_child_text();
	}

	static inline void set_first_child_text(Element* element, const Glib::ustring& content) {
		return element->set_child_text(content);
	}
#else  // Version 3
	typedef Node::const_NodeSet const_NodeSet; // correct libxml++ 3.0 implementation

	static inline Element* add_child_element(Element* element, const Glib::ustring& name) {
		return element->add_child_element(name);
	}

	static inline const TextNode* get_first_child_text(const Element& element) {
		return element.get_first_child_text();
	}

	static inline void set_first_child_text(Element* element, const Glib::ustring& content) {
		return element->set_first_child_text(content);
	}
#endif
}
