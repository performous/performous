#include "control.hh"

Control::Control(Control* parent)
: m_parent(parent) {
}

Control* Control::getParent() {
	return m_parent;
}

Control const* Control::getParent() const {
	return m_parent;
}

void Control::setParent(Control* parent) {
	m_parent = parent;
}

void Control::setGeometry(float x, float y, float width, float height) {
	m_x = x;
	m_y = y;
	m_width = width;
	m_height = height;
}

float Control::getX() const {
	return m_x;
}

float Control::getY() const {
	return m_y;
}

float Control::getWidth() const {
	return m_width;
}

float Control::getHeight() const {
	return m_height;
}

void Control::setName(std::string const& name) {
	m_name = name;
}

std::string Control::getName() const {
	return m_name;
}
