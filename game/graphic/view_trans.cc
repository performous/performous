#include "view_trans.hh"

#include "video_driver.hh"
#include "window.hh"

ViewTrans::ViewTrans(Window& window, float offsetX, float offsetY, float frac)
: m_window(window), m_old(Global::projection) {
	// Setup the projection matrix for 2D translates
	using namespace glmath;
	float h = virtH();
	const float f = Constant::nearDistance / Constant::z0;  // z0 to nearplane conversion factor
	// Corners of the screen at z0
	float x1 = -0.5f, x2 = 0.5f;
	float y1 = 0.5f * h, y2 = -0.5f * h;
	// Move the perspective point by frac of offset (i.e. move the image)
	float persX = frac * offsetX, persY = frac * offsetY;
	x1 -= persX; x2 -= persX;
	y1 -= persY; y2 -= persY;
	// Perspective projection + the rest of the offset in eye (world) space
	Global::projection = glm::frustum(f * x1, f * x2, f * y1, f * y2, Constant::nearDistance, Constant::farDistance)
	  * translate(vec3(offsetX - persX, offsetY - persY, -Constant::z0));
	window.updateTransforms();
}

ViewTrans::ViewTrans(Window& window, glmath::mat4 const& m)
: m_window(window), m_old(Global::projection) {
	Global::projection = Global::projection * m;
	m_window.updateTransforms();
}

ViewTrans::~ViewTrans() {
	Global::projection = m_old;
	m_window.updateTransforms();
}
