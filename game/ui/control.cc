#include "control.hh"

#include <iostream>

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

void Control::enable() {
	m_enabled = true;
}

void Control::disable() {
	m_enabled = false;
}

void Control::setEnabled(bool enabled) {
	m_enabled = enabled;
}

bool Control::isEnabled() const {
	return m_enabled;
}

void Control::setTabIndex(unsigned index) {
	m_tabIndex = index;
}

unsigned Control::getTabIndex() const {
	return m_tabIndex;
}

void Control::onKeyDown(std::function<void(Control&, Key)> const& callback) {
	m_onKeyDown = callback;
}

void Control::onKeyUp(std::function<void(Control&, Key)> const& callback) {
	m_onKeyUp = callback;
}

void Control::sendOnKeyDown(Key key) {
	std::cout << "sendOnKeyDown" << std::endl;
	if(m_onKeyDown)
		m_onKeyDown(*this, key);
}

void Control::sendOnKeyUp(Key key) {
	if(m_onKeyUp)
		m_onKeyUp(*this, key);
}

void Control::drawFocus() {
	if(hasFocus()) {
		m_focus.setGeometry(getX(), getY(), getWidth(), getHeight());
		m_focus.draw();
	}
}
