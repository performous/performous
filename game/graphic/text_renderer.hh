#pragma once

#include "size.hh"
#include "opengl_text.hh"

#include <string>

class TextRenderer {
public:
	OpenGLText render(std::string const&, TextStyle const&, float m);
	Size measure(std::string const&, TextStyle const&, float m);

private:
    void renderTextFill(std::shared_ptr<cairo_t> dc, TextStyle const& style, bool complete);  ///< Draw the text inside fill
    void renderTextStroke(std::shared_ptr<cairo_t> dc, TextStyle const& style, float border, bool complete); ///< Draw the text outline stroke

};

