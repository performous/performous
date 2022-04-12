#include "label.hh"
#include "graphiccontext.hh"

Label::Label(std::string const& text, Control* parent)
: Control(parent), m_text(text) {
}

Label::Label(Control* parent, std::string const& text)
: Control(parent), m_text(text) {
}

void Label::setText(std::string const& text) {
	m_text.setText(text);
}

std::string Label::getText() const {
	return m_text.getText();
}

void Label::draw(GraphicContext& gc) {
	gc.draw(m_text, getX(), getY(), getWidth(), getHeight());
}
