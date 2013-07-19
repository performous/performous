#pragma once

#include "glmath.hh"
#include <istream>

/// A struct for holding RGBA color in non-premultiplied linear RGB format (and conversions)
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
	/// Parse CSS color string (sRGB)
	explicit Color(std::string const& str);
	/// Return premultiplied linear color suitable for use with OpenGL
	glmath::vec4 linear() const;
};

std::istream& operator>>(std::istream& is, Color& color);

