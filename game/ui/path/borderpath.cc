#include "ui/path/borderpath.hh"

#include <cmath>
#include <stdexcept>

BorderPath::BorderPath(float radius)
	: m_radius(radius) {
}

void BorderPath::setGeometry(float x, float y, float width, float height) {
	m_x = x;
	m_y = y;
	m_width = width;
	m_height = height;
}

float BorderPath::getX() const {
	return m_x;
}

float BorderPath::getY() const {
	return m_y;
}

float BorderPath::getWidth() const {
	return m_width;
}

float BorderPath::getHeight() const {
	return m_height;
}

namespace {
	float const pi(atanf(1.f) * 4.f);
}

float BorderPath::getLength() const {
	if(m_radius == 0.f)
		return m_width * 2.f + m_height * 2.f;

	return (m_width - m_radius * 2.0f) * 2.f + (m_height - m_radius * 2.f) * 2.f + 2.f * pi * m_radius;
}

Point BorderPath::getPoint(float location) const {
	auto l = location * getLength();

	if (m_radius == 0.f) {
		if (l < m_width)
			return Point(m_x + l, m_y);
		if (l < m_width + m_height)
			return Point(m_x + m_width, m_y + (l - m_width));
		if (l < m_width + m_height + m_width)
			return Point(m_x + m_width - (l - m_width - m_height), m_y + m_height);
		return Point(m_x, m_y + m_height - (l - m_width - m_height - m_width));
	}

	throw std::logic_error("Not implemented yet!");
}
