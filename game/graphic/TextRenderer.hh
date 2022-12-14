#pragma once

#include "opengl_text.hh"

#include <string>

class TextRenderer {
public:
	OpenGLText render(std::string const&, TextStyle&, float m);
};

