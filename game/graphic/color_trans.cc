#include "color_trans.hh"

#include "color.hh"
#include "window.hh"
#include "video_driver.hh"

ColorTrans::ColorTrans(Window& window, glmath::mat4 const& mat)
: m_window(window), m_old(Global::color) {
	Global::color = Global::color * mat;
	window.updateColor();
}

ColorTrans::ColorTrans(Window& window, Color const& c)
 : m_window(window), m_old(Global::color) {
	using namespace glmath;
	Global::color = Global::color * diagonal(c.linear());
	window.updateColor();
}

ColorTrans::~ColorTrans() {
	Global::color = m_old;
	m_window.updateColor();
}
