#include "color.hh"
#include "log.hh"

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
		if (sRGB <= 0.04045f) return sRGB / 12.92f;
		return std::pow((sRGB + 0.055f)/1.055f, 2.4f);
	}
}

Color::Color(std::string const& str) {
	int r = 0, g = 0, b = 0, a = 255;
	if (str.size() > 0 && str[0] == '#' && sscanf(str.c_str() + 1, "%02x %02x %02x %02x", &r, &g, &b, &a) >= 3) {
		*this = Color(lin(static_cast<float>(r) / 255.0f), lin(static_cast<float>(g) / 255.0f), lin(static_cast<float>(b) / 255.0f), static_cast<float>(a) / 255.0f);
		return;
	}
	ColorNames::Map::const_iterator it = colors.m.find(str);
	if (it != colors.m.end()) { *this = it->second; return; }
	SpdLogger::warn(LogSystem::IMAGE, "Unknown color: {}; using magenta to highlight.");
	*this = Color(1.0f, 0.0f, 1.0f, 1.0f);
}

glmath::vec4 Color::linear() const {
	return a * glmath::vec4(r, g, b, 1.0f);
}

bool Color::operator==(Color const& other) const {
	return r == other.r && g == other.g && b == other.b && a == other.a;
}

bool Color::operator!=(Color const& other) const {
	return !operator==(other);
}
