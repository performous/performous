#pragma once
#include <iostream>

namespace Unicode {
	void convert( std::stringstream &_stream, std::string _filename );
	std::string collate(std::string const& str);
}
