#pragma once

#include "glmath.hh"

struct Color;
class Window;

struct ColorTrans {
	ColorTrans(Window&, Color const& c);
	ColorTrans(Window&, glmath::mat4 const& mat);
	~ColorTrans();

  private:
	Window& m_window;
	glmath::mat4 m_old;
};
