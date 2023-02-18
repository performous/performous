#pragma once

#include "glmath.hh"

class Window;

/// Apply a transform to current modelview stack
class Transform {
  public:
	Transform(Window&, glmath::mat4 const& m);
	~Transform();

  private:
	Window& m_window;
	glmath::mat4 m_old;
};
