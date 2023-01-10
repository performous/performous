#pragma once

#include "size.hh"
#include "opengl_text.hh"

#include <string>

class TextRenderer {
public:
	OpenGLText render(std::string const&, TextStyle const&, float m);
	Size measure(std::string const&, TextStyle const&, float m);
};

