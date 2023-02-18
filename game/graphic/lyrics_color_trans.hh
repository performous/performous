#pragma once

#include "glmath.hh"

struct Color;
class Window;

struct LyricColorTrans {
	LyricColorTrans(Window&, Color const& fill, Color const& stroke, Color const& newFill, Color const& newStroke);
	~LyricColorTrans();

  private:
	Window& m_window;
	glmath::vec4 oldFill;
	glmath::vec4 oldStroke;
};
