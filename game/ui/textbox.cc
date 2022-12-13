#include "textbox.hh"
#include "graphiccontext.hh"

TextBox::TextBox(std::string const& text, Control* parent)
: Control(parent), m_text(text), m_background(findFile("mainmenu_back_highlight.svg")), m_cursor(findFile("cursor.svg")) {
}

TextBox::TextBox(Control* parent, std::string const& text)
: Control(parent), m_text(text),  m_background(findFile("mainmenu_back_highlight.svg")), m_cursor(findFile("cursor.svg")) {
}

TextBox& TextBox::setText(std::string const& text, bool keepCursorPosition) {
	if(m_text.getText() != text) {
		if(m_onTextChanged)
			m_onTextChanged(*this, text, m_text.getText());

		m_text.setText(text);
	}

	if(keepCursorPosition) {
		if(m_cursorPosition > text.size())
			m_cursorPosition = text.size();
	}
	else
		m_cursorPosition = text.size();

	return *this;
}

std::string TextBox::getText() const {
	auto text = m_text.getText();

	if(m_cursorPosition < text.size() && text[m_cursorPosition] == '|')
		text.erase(m_cursorPosition, 1);

	return text;
}

void TextBox::onTextChanged(std::function<void(TextBox&, std::string const&, std::string const&)> const& callback) {
	m_onTextChanged = callback;
}

TextBox& TextBox::setMaxLength(size_t length) {
	m_maxLength = length;

	return *this;
}

size_t TextBox::getMaxLength() const {
	return m_maxLength;
}

namespace {
	char convertToChar(Control::Key key, Control::Key base, char offset) {
		return static_cast<char>(static_cast<int>(key) - static_cast<int>(base) + offset);
	}
}

void TextBox::onKey(Key key) {
	auto text = getText();

	switch(key) {
		case Key::Up:
		case Key::Left:
			if(m_cursorPosition > 0)
				--m_cursorPosition;
			break;
		case Key::Down:
		case Key::Right:
			if(m_cursorPosition < text.size())
				++m_cursorPosition;
			break;
		case Key::BackSpace:
			if(m_cursorPosition > 0)
				text.erase(--m_cursorPosition, 1);
			break;
		case Key::Delete:
			if(m_cursorPosition < text.size())
				text.erase(m_cursorPosition, 1);
			break;
		default:
			if(text.length() < m_maxLength) {
				if(key >= Key::Number0 && key <= Key::Number9)
					text.insert(m_cursorPosition++, 1, convertToChar(key, Key::Number0, '0'));
				else if(key >= Key::a && key <= Key::z)
					text.insert(m_cursorPosition++, 1, convertToChar(key, Key::a, 'a'));
				else if(key >= Key::A && key <= Key::Z)
					text.insert(m_cursorPosition++, 1, convertToChar(key, Key::A, 'A'));
				else if(key == Key::Space)
					text.insert(m_cursorPosition++, 1, ' ');
			}
	}

	setText(text, true);
}

void TextBox::draw(GraphicContext& gc) {
	drawFocus(gc);

	const auto color = ColorTrans(gc.getWindow(), hasFocus() ? Color(1.f, 1.f, 1.f) : Color(0.6f, 0.6f, 0.6f));
	auto text = m_text.getText();

	m_background.dimensions.left(getX()).top(getY()).stretch(getWidth(), getHeight());
	m_background.draw(gc.getWindow());

	m_text.setText(text);

	gc.draw(m_text, getX(), getY(), getWidth(), getHeight());

	if(hasFocus()) {
		// draw cursor
		auto const textBeforeCursor = m_text.getText().substr(0, m_cursorPosition);
		auto const width = m_text.measure(textBeforeCursor).width * m_text.getWidth();
		auto const x = getX() + width;
		auto const y = getY() + getHeight() * 0.05f;
		auto const w = getHeight() * 0.9f * 0.05f;
		auto const h = getHeight() * 0.9f;

		m_cursor.dimensions.left(x).top(y).stretch(w, h);
		//m_cursor.dimensions.left(-0.05).top(-0.05).stretch(0.1f, 0.1f);
		m_cursor.draw(gc.getWindow());
	}
}

