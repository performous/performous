#pragma once
#ifndef PERFORMOUS_COLOR_HH
#define PERFORMOUS_COLOR_HH

#include <istream>
#include <map>
#include <string>

/// color struct
struct Color {
	/// red part
	double r;
	/// green part
	double g;
	/// blue part
	double b;
	/// alpha part
	double a;
	/// constructor
	Color(double red = 0.0, double grn = 0.0, double blu = 0.0, double alp = 1.0): r(red), g(grn), b(blu), a(alp) {}
	/** Parse CSS color string **/
	Color(std::string const& str);
};

static inline Color getColor(std::istream& is) {
	std::string str;
	std::getline(is, str);
	return Color(str);
}

#endif
