#include "button.hh"

#include "graphiccontext.hh"

Button::Button(std::string const& text, Control* parent)
: Control(parent), m_text(text) {
}

Button::Button(Control* parent, std::string const& text)
: Button(text, parent) {
}

void Button::setText(std::string const& text) {
	m_text.setText(text);
}

std::string Button::getText() const {
	return m_text.getText();
}

void Button::onClicked(const std::function<void (Button&)>& callback) {
	m_onClicked = callback;
}

void Button::onKey(Key key) {
	if((key == Key::Space || key == Key::Return) && m_onClicked)
		m_onClicked(*this);
}

void Button::draw(GraphicContext& gc) {
	drawFocus(gc);

	if (!m_background)
		m_background = gc.getTheme().getButtonBG();

	m_background->dimensions.left(getX()).top(getY()).stretch(getWidth(), getHeight());
	m_background->draw(gc.getWindow());

	gc.drawCentered(m_text, getX(), getY(), getWidth(), getHeight());
}


