#pragma once

#include "size.hh"
#include "opengl_text.hh"
#include <string>

class TextRenderer {
public:
	static const size_t MAX_COLUMNS{16};  // The maximum number of columns we will measure

	OpenGLText render(const std::string &, const TextStyle&, float m);
	Size measure(const std::string &, const TextStyle&, float m);
	size_t measureColumns(const std::string& text, const TextStyle& style, float m, std::array<float,MAX_COLUMNS>& columns);
};

