#include "transform.hh"

#include "window.hh"
#include "video_driver.hh"

Transform::Transform(Window& window, glmath::mat4 const& m)
 : m_window(window), m_old(Global::modelview) {
	Global::modelview = Global::modelview * m;
	window.updateTransforms();
}

Transform::~Transform() {
	Global::modelview = m_old;
	m_window.updateTransforms();
}
