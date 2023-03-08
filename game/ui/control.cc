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

	auto drawer =  std::make_shared<TextureDrawer>(m_focusEffectImage);
	auto pathChaser = std::make_shared<PathChaser>(m_focus);

	pathChaser->setConsumer([&](Point const& point) {
		//std::cout << "consumer: " << point << std::endl;
		//m_focusEffectImage.dimensions.left(point.getX() - 0.005f).top(point.getY() - 0.005f);
		auto v = glmath::vec2();

		v[0] = point.getX();
		v[1] = point.getY();

		m_matrix = glm::mat3(1.f);
		m_matrix = glm::scale(m_matrix, glmath::vec2(0.33f));
		m_matrix = glm::translate(m_matrix, v);
		//m_matrix = glm::scale(m_matrix, glmath::vec2(0.25f));
	});

	auto rotation = std::make_shared<Rotation>();

	rotation->setConsumer([&](float angle) {
		//std::cout << "consumer: " << angle << std::endl;
		//m_matrix = glm::mat3(1.f);
		m_matrix = glm::rotate(m_matrix, angle);
	});

	auto generic = std::make_shared<Generic>([&, drawer](EffectContext& context){
		//std::cout << "generic" << std::endl;
		const auto v = context.getSecondsSinceStart() * 0.5f;
		const auto a = v - floor(v);
		drawer->setMatrix(m_matrix);
		drawer->setColor(Color(1.f, 0.f, 0.f, a));
	});

	auto border = std::make_shared<Sinus>(1.f, 0.f, 1.f);

	border->setConsumer([&](float value, EffectContext& context){
		//std::cout << "border: " << v << std::endl;
		auto const color = ColorTrans(context.getGraphicContext().getWindow(), Color::alpha(value));
		m_focus.draw(context.getGraphicContext().getWindow());
	});

	m_focusEffect = combine(border/*, pathChaser, rotation, generic, drawer*/);
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

void Control::drawFocus(GraphicContext& gc) {
	if(hasFocus()) {
		m_focus.setGeometry(getX(), getY(), getWidth(), getHeight());

		gc.add(m_focusEffect);
	}
	else {
		gc.remove(m_focusEffect);
	}
}
