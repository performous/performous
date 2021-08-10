#pragma once

#include "config.hh"

#include <libxml++/libxml++.h>

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

	static inline Element* add_child_element(Element* element, const strType& name) {
		#if LIBXMLPP_VERSION_2_6
			return element->add_child(name);
		#else
			return element->add_child_element(name);
		#endif
	}

	static inline const TextNode* get_first_child_text(const Element& element) {
		#if LIBXMLPP_VERSION_2_6
			return element.get_child_text();
		#else
			return element.get_first_child_text();
		#endif
	}

	static inline void set_first_child_text(Element* element, const strType& content) {
		#if LIBXMLPP_VERSION_2_6
			return element->set_child_text(content);
		#else
			return element->set_first_child_text(content);
		#endif
	}
}