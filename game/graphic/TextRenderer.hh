#pragma once

#include "opengl_text.hh"

class TextRenderer {
public:
	OpenGLText render(std::string const&, TextStyle const&, float m);
};

