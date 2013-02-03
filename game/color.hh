#pragma once

#include "glmath.hh"
#include <istream>
#include <string>

/// A struct for holding RGBA color in non-premultiplied non-linear format (sRGB)
struct Color {
	double r;
	double g;
	double b;
	double a;
	/// Default-construct white
	Color(): r(1.0), g(1.0), b(1.0), a(1.0) {}
	/// Construct using RGB(A)
	Color(double red, double grn, double blu, double alp = 1.0): r(red), g(grn), b(blu), a(alp) {}
	/// Construct white color with alpha
	static Color alpha(double alp) { return Color(1.0, 1.0, 1.0, alp); }
	/// Parse CSS color string
	explicit Color(std::string const& str);
	/// Return pre-multiplied linear color suitable for use with OpenGL
	glmath::vec4 linear() const;
};

static inline Color getColor(std::istream& is) {
	std::string str;
	std::getline(is, str);
	return Color(str);
}
