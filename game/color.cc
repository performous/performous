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
			m["none"] = Color("#00000000");
			m["black"] = Color("#000000");
			m["gray"] = Color("#808080");
			m["silver"] = Color("#C0C0C0");
			m["white"] = Color("#FFFFFF");
			m["maroon"] = Color("#800000");
			m["red"] = Color("#FF0000");
			m["green"] = Color("#008000");
			m["lime"] = Color("#00FF00");
			m["navy"] = Color("#000080");
			m["blue"] = Color("#0000FF");
			m["purple"] = Color("#800080");
			m["fuchsia"] = Color("#FF00FF");
			m["olive"] = Color("#808000");
			m["yellow"] = Color("#FFFF00");
			m["teal"] = Color("#008080");
			m["aqua"] = Color("#00FFFF");
		}
	} colors;

	// Convert sRGB color component into linear as per OpenGL specs
	double lin(double sRGB) {
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
	std::clog << "Color/warning: Unknown color: " << str << " (using magenta to hilight)" << std::endl;
	*this = Color(1.0, 0.0, 1.0);
}

glmath::vec4 Color::linear() const {
	return a * glmath::vec4(r, g, b, 1.0);
}

Color MicrophoneColor::get(std::string name) {
	if (name == "blue") return Color(0.0, 128.0/255.0, 1.0);
	else if (name == "red") return Color(1, 0.0, 0.0);
	else if (name == "green") return Color(0.0, 1.0, 0.0);
	else if (name == "yellow") return Color(1.0, 1.0, 0.0);
	else if (name == "fuchsia") return Color(128.0/255.0, 0.0, 1.0);
	else if (name == "orange") return Color(1.0, 85.0/255, 0.0);
	else if (name == "purple") return Color(132.0/255.0, 0, 1.0);
	else if (name == "aqua") return Color(0.0, 1.0, 1.0);
	else return Color(0.5, 0.5, 0.5);
}

