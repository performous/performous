#include "color.hh"
#include <cstdio>
#include <iostream>
#include <map>

namespace {
	struct ColorNames {
		typedef std::map<std::string, Color> Map;
		Map m;
		ColorNames() {
			m["none"] = Color(0.0, 0.0, 0.0, 0.0);
			m["black"] = Color(0.0, 0.0, 0.0);
			m["gray"] = Color(0.5, 0.5, 0.5);
			m["silver"] = Color(0.75, 0.75, 0.75);
			m["white"] = Color(1.0, 1.0, 1.0);
			m["maroon"] = Color(0.5, 0.0, 0.0);
			m["red"] = Color(1.0, 0.0, 0.0);
			m["green"] = Color(0.0, 0.5, 0.0);
			m["lime"] = Color(0.0, 1.0, 0.0);
			m["navy"] = Color(0.0, 0.0, 0.5);
			m["blue"] = Color(0.0, 0.0, 1.0);
			m["purple"] = Color(0.5, 0.0, 0.5);
			m["fuchsia"] = Color(1.0, 0.5, 1.0);
			m["olive"] = Color(0.5, 0.5, 0.0);
			m["yellow"] = Color(1.0, 1.0, 0.0);
			m["teal"] = Color(0.0, 0.5, 0.5);
			m["aqua"] = Color(0.0, 1.0, 1.0);
		}
	} colors;
}

Color::Color(std::string const& str) {
	unsigned int r = 0, g = 0, b = 0;
	if (str.size() == 7 && str[0] == '#' && sscanf(str.c_str() + 1, "%02x %02x %02x", &r, &g, &b) == 3) {
		*this = Color(r / 255.0, g / 255.0, b / 255.0);
		return;
	}
	ColorNames::Map::const_iterator it = colors.m.find(str);
	if (it != colors.m.end()) { *this = it->second; return; }
	std::cerr << "WARNING: Unknown color: " << str << " (using magenta to hilight)" << std::endl;
	*this = Color(1.0, 0.0, 1.0);
}

