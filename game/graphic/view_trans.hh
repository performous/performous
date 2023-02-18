#pragma once

#include "glmath.hh"

class Window;

class ViewTrans {
  public:
	/// Apply a translation on top of current viewport translation
	ViewTrans(Window&, glmath::mat4 const& m);
	/// Apply a subviewport with different perspective projection
	ViewTrans(Window&, float offsetX = 0.0f, float offsetY = 0.0f, float frac = 1.0f);
	~ViewTrans();

 private:
	Window& m_window;
	glmath::mat4 m_old;
};
