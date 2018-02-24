#pragma once

#include <libxml++/libxml++.h>

namespace xmlpp {
	// typedef Node::const_NodeSet const_NodeSet; // correct libxml++ 3.0 implementation
	typedef NodeSet const_NodeSet; // implementation to satisfy libxml++ 2.6 API
}
