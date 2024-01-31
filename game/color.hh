#pragma once

#include "graphic/glmath.hh"

#include <istream>

/// A struct for holding RGBA color in non-premultiplied linear RGB format (and conversions)
struct Color {
	float r;
	float g;
	float b;
	float a;
	/// Default-construct white
	Color(): r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
	/// Construct using RGB(A)
	Color(float red, float grn, float blu, float alp = 1.0f): r(red), g(grn), b(blu), a(alp) {}
	/// Construct white color with alpha
	static Color alpha(float alp) { return Color(1.0f, 1.0f, 1.0f, alp); }
	/// Parse CSS color string (sRGB)
	explicit Color(std::string const& str);
	/// Return premultiplied linear color suitable for use with OpenGL
	glmath::vec4 linear() const;

	bool operator==(Color const&) const;
	bool operator!=(Color const&) const;
};

std::istream& operator>>(std::istream& is, Color& color);

