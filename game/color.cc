#include "color.hh"
#include <cstdio>
#include <iostream>
#include <map>
#include <string>

std::istream& operator>>(std::istream& is, Color& color) {
	std::string str;
	is >> str;
	color = Color(str);
	return is;
}

namespace {
	struct ColorNames {
		typedef std::map<std::string, Color> Map;
		Map m;
		ColorNames() {
			m["maroon"] = Color("#800000FF");
			m["red"] = Color("#FF0000FF");
			m["green"] = Color("#008000FF");
			m["lime"] = Color("#00FF00FF");
			m["navy"] = Color("#000080FF");
			m["blue"] = Color("#0000FFFF");
			m["purple"] = Color("#800080FF");
			m["fuchsia"] = Color("#FF00FFFF");
			m["olive"] = Color("#808000FF");
			m["yellow"] = Color("#FFFF00FF");
			m["teal"] = Color("#008080FF");
			m["aqua"] = Color("#00FFFFFF");
			m["white"] = Color("#FFFFFFFF");
			m["none"] = Color("#00000000");
			m["black"] = Color("#000000FF");
			m["gray"] = Color("#808080FF");
			m["silver"] = Color("#C0C0C0FF");
			}
	} colors;

	// Convert sRGB color component into linear as per OpenGL specs
	float lin(float sRGB) {
		if (sRGB <= 0.04045) return sRGB / 12.92;
		return std::pow((sRGB + 0.055)/1.055, 2.4);
	}
}

Color::Color(std::string const& str) {
	unsigned int r = 0, g = 0, b = 0, a = 255;
	if (str.size() > 0 && str[0] == '#' && sscanf(str.c_str() + 1, "%02x %02x %02x %02x", &r, &g, &b, &a) >= 3) {
		*this = Color(lin(r / 255.0), lin(g / 255.0), lin(b / 255.0), a / 255.0);
		return;
	}
	ColorNames::Map::const_iterator it = colors.m.find(str);
	if (it != colors.m.end()) { *this = it->second; return; }
	std::clog << "color/warning: Unknown color: " << str << " (using magenta to highlight)" << std::endl;
	*this = Color(1.0, 0.0, 1.0, 1.0);
}

glmath::vec4 Color::linear() const {
	return a * glmath::vec4(r, g, b, 1.0);
}

Color MicrophoneColor::get(std::string name) {
	if (name == "black") return Color(3.0/255.0, 3.0/255.0, 3.0/255.0, 1.0);
	else if (name == "gray") return Color(24.0/255.0, 24.0/255.0, 24.0/255.0, 1.0);
	else if (name == "white") return Color(1.0, 1.0, 1.0, 1.0);
	else if (name == "aqua") return Color(0.0, 1.0, 1.0, 1.0);
	else if (name == "purple") return Color(63.0/255.0, 0.0, 1.0, 1.0);
	else if (name == "orange") return Color(1.0, 52.0/255.0, 0.0, 1.0);
	else if (name == "fuchsia") return Color(1.0, 0.06, 127/255.0, 1.0);
	else if (name == "yellow") return Color(1.0, 1.0, 0.0, 1.0);
	else if (name == "green") return Color(0.0, 1.0, 0.0, 1.0);	
	else if (name == "red") return Color(1, 0.0, 0.0, 1.0);
	else if (name == "blue") return Color(0.0, 43.75/255.0, 1.0, 1.0);
	else return Color(0.5, 0.5, 0.5, 1.0);
}
