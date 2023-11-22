#include "dimensions.hh"

#include <stdexcept>
#include "window.hh"


Dimensions::Dimensions(float ar_)
	: m_ar(ar_), m_x(), m_y(), m_w(), m_h(), m_xAnchor(), m_yAnchor(), m_screenAnchor() {
}

Dimensions::Dimensions(float x1, float y1, float w, float h)
	: m_ar(), m_x(x1), m_y(y1), m_w(w), m_h(h),
	m_xAnchor(XAnchor::LEFT), m_yAnchor(YAnchor::TOP), m_screenAnchor() {
}

Dimensions& Dimensions::middle(float x) {
	m_x = x;
	m_xAnchor = XAnchor::MIDDLE;

	return *this;
}

Dimensions& Dimensions::left(float x) {
	m_x = x;
	m_xAnchor = XAnchor::LEFT;
	return *this;
}

Dimensions& Dimensions::right(float x) {
	m_x = x;
	m_xAnchor = XAnchor::RIGHT;
	return *this;
}

Dimensions& Dimensions::center(float y) {
	m_y = y;
	m_yAnchor = YAnchor::CENTER;
	return *this;
}

Dimensions& Dimensions::top(float y) {
	m_y = y;
	m_yAnchor = YAnchor::TOP;
	return *this;
}

Dimensions& Dimensions::bottom(float y) {
	m_y = y;
	m_yAnchor = YAnchor::BOTTOM;
	return *this;
}

Dimensions& Dimensions::ar(float ar) {
	m_ar = ar;

	return fixedWidth(m_w);
}

Dimensions& Dimensions::fixedWidth(float w) {
	m_w = w;
	m_h = w / m_ar;
	return *this;
}

Dimensions& Dimensions::fixedHeight(float h) {
	m_w = h * m_ar;
	m_h = h;
	return *this;
}

Dimensions& Dimensions::fitInside(float w, float h) {
	if (w / h > m_ar)
		fixedHeight(h);
	else
		fixedWidth(w);
	return *this;
}

Dimensions& Dimensions::fitOutside(float w, float h) {
	if (w / h > m_ar)
		fixedWidth(w);
	else
		fixedHeight(h);
	return *this;
}

Dimensions& Dimensions::stretch(float w, float h) {
	m_w = w;
	m_h = h;
	m_ar = w / h;
	return *this;
}

Dimensions& Dimensions::screenCenter(float y) {
	m_screenAnchor = YAnchor::CENTER;
	center(y);
	return *this;
}

Dimensions& Dimensions::screenTop(float y) {
	m_screenAnchor = YAnchor::TOP;
	top(y);
	return *this;
}

Dimensions& Dimensions::screenBottom(float y) {
	m_screenAnchor = YAnchor::BOTTOM;
	bottom(y);
	return *this;
}

Dimensions& Dimensions::move(float x, float y) {
	m_x += x;
	m_y += y;
	return *this;
}

Dimensions& Dimensions::scale(float f) {
	m_scaleHorizontal = m_scaleVertical = f;
	return *this;
}

Dimensions& Dimensions::scale(float horizontal, float vertical) {
	m_scaleHorizontal = horizontal;
	m_scaleVertical = vertical;
	return *this;
}

float Dimensions::x1() const {
	switch (m_xAnchor) {
	case XAnchor::LEFT: return m_x;
	case XAnchor::MIDDLE: return m_x - 0.5f * w();
	case XAnchor::RIGHT: return m_x - w();
	}
	throw std::logic_error("Unknown value in Dimensions::m_xAnchor");
}

float Dimensions::y1() const {
	switch (m_yAnchor) {
	case YAnchor::TOP: return screenY() + m_y;
	case YAnchor::CENTER: return screenY() + m_y - 0.5f * h();
	case YAnchor::BOTTOM: return screenY() + m_y - h();
	}
	throw std::logic_error("Unknown value in Dimensions::m_yAnchor");
}

float Dimensions::ar() const {
	return m_ar;
}

float Dimensions::x2() const {
	return x1() + w();
}

float Dimensions::y2() const {
	return y1() + h();
}

float Dimensions::xc() const {
	return x1() + 0.5f * w();
}

float Dimensions::yc() const {
	return y1() + 0.5f * h();
}

float Dimensions::w() const {
	return m_w * m_scaleHorizontal;
}

float Dimensions::getWidth(bool scaled) const {
	if (scaled)
		return m_w * m_scaleHorizontal;
	return m_w;
}

float Dimensions::h() const {
	return m_h * m_scaleVertical;
}

float Dimensions::getHeight(bool scaled) const {
	if (scaled)
		return m_h * m_scaleVertical;
	return m_h;
}

float Dimensions::screenY() const {
	switch (m_screenAnchor) {
	case YAnchor::CENTER: return 0.0f;
	case YAnchor::TOP: return -0.5f * virtH();
	case YAnchor::BOTTOM: return 0.5f * virtH();
	}
	throw std::logic_error("Dimensions::screenY(): unknown m_screenAnchor value");
}

