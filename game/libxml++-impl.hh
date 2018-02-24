#pragma once

#include <libxml++/libxml++.h>

namespace xmlpp {
	// typedef Node::const_NodeSet const_NodeSet; // correct libxml++ 3.0 implementation
	typedef NodeSet const_NodeSet; // implementation to satisfy libxml++ 2.6 API

	static inline Element* add_child_element(Element* element, const Glib::ustring& name) {
		return element->add_child(name);
	}
}
