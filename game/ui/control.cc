#include "control.hh"

#include "ui/effect/pathchaser.hh"
#include "ui/effect/rotation.hh"
#include "ui/effect/texturedrawer.hh"
#include "ui/effect/sinus.hh"
#include "ui/effect/generic.hh"
#include "ui/effect/combination.hh"
#include "ui/graphiccontext.hh"
#include "graphic/color_trans.hh"

#include "fs.hh"

#include <glm/gtx/matrix_transform_2d.hpp>

#include <iostream>

Control::Control(Control* parent)
: m_parent(parent), m_focusEffectImage(findFile("star_glow.svg")) {
	m_focusEffectImage.dimensions.stretch(0.01f, 0.01f);
	m_focusEffectImage.dimensions.left(-0.005f).top(-0.005f);

	auto border = std::make_shared<Sinus>(1.f, 0.f, 1.f);

	border->setConsumer([&](float value, EffectContext& context){
		auto const color = ColorTrans(context.getGraphicContext().getWindow(), Color::alpha(value));
		m_focus->draw(context.getGraphicContext().getWindow());
	});

	m_focusEffect = combine(border);
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
	if(m_onKeyDown)
		m_onKeyDown(*this, key);
}

void Control::sendOnKeyUp(Key key) {
	if(m_onKeyUp)
		m_onKeyUp(*this, key);
}

void Control::drawFocus(GraphicContext& gc) {
	if (!m_focus)
		m_focus = std::make_unique<Border>(gc.getTheme().getFocus());

	if(hasFocus()) {
		m_focus->setGeometry(getX(), getY(), getWidth(), getHeight());

		gc.add(m_focusEffect);
	}
	else {
		gc.remove(m_focusEffect);
	}
}
