#pragma once

#include "size.hh"
#include "opengl_text.hh"
#include <string>
#include <array>

using TextExtent = std::pair<size_t,size_t>; // The location and length of a substring

class TextRenderer {
public:
	static const size_t MAX_COLUMNS{32};  // The maximum number of lines/columns we will measure

	OpenGLText render(const std::string &, const TextStyle&, float m);
	Size measure(const std::string &, const TextStyle&, float m);
	size_t measureColumnsOLD(const std::string& text, const TextStyle& style, float m, std::array<float,MAX_COLUMNS>& columns);
	size_t measureColumns(const std::string& text, const TextStyle& style, float m, std::array<float,MAX_COLUMNS>& columns);

    size_t tokenSplitter(const std::string &text, char token, std::array<TextExtent,MAX_COLUMNS> &extents); ///< Finds extents of token-delimited substrings

};

