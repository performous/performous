#include "textbox.hh"
#include "graphiccontext.hh"

TextBox::TextBox(std::string const& text, Control* parent)
: Control(parent), m_text(text),  m_background(findFile("mainmenu_back_highlight.svg")) {
}

TextBox::TextBox(Control* parent, std::string const& text)
: Control(parent), m_text(text),  m_background(findFile("mainmenu_back_highlight.svg")) {
}

void TextBox::sendTextChanged(std::string const& newText, std::string const& oldText) {
	if(oldText != newText) {
		if(m_onTextChanged)
			m_onTextChanged(*this, newText, oldText);
	}
}

TextBox& TextBox::setText(std::string const& text) {
	if(m_text.getText() != text) {
		if(m_onTextChanged)
			m_onTextChanged(*this, text, m_text.getText());

		m_text.setText(text);
	}

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
	auto text = m_text.getText();

	if(m_cursorPosition < text.size() && text[m_cursorPosition] == '|')
		text.erase(m_cursorPosition, 1);

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
				const auto oldText = getText();

				if(key >= Key::Number0 && key <= Key::Number9)
					text.insert(m_cursorPosition++, 1, convertToChar(key, Key::Number0, '0'));
				else if(key >= Key::a && key <= Key::z)
					text.insert(m_cursorPosition++, 1, convertToChar(key, Key::a, 'a'));
				else if(key >= Key::A && key <= Key::Z)
					text.insert(m_cursorPosition++, 1, convertToChar(key, Key::A, 'A'));
				else if(key == Key::Space)
					text.insert(m_cursorPosition++, 1, ' ');

				sendTextChanged(getText(), oldText);
			}
	}

	if(m_cursorPosition < text.size() && text[m_cursorPosition] != '|')
		text.insert(m_cursorPosition, 1, '|');
	else if(m_cursorPosition == text.size())
		text.append(1, '|');

	m_text.setText(text);
}

void TextBox::draw(GraphicContext& gc) {
	drawFocus();

	const auto color = ColorTrans(hasFocus() ? Color(1.f, 1.f, 1.f) : Color(0.6f, 0.6f, 0.6f));

	m_background.dimensions.left(getX()).top(getY()).stretch(getWidth(), getHeight());
	m_background.draw();

	if(hasFocus()) {
		auto text = m_text.getText();

		if(m_cursorPosition < text.size() && text[m_cursorPosition] != '|')
			text.insert(m_cursorPosition, 1, '|');
		else if(m_cursorPosition == text.size())
			text.append(1, '|');

		m_text.setText(text);
	}
	else {
		auto text = m_text.getText();

		if(m_cursorPosition < text.size() && text[m_cursorPosition] == '|')
			text.erase(m_cursorPosition, 1);

		m_text.setText(text);
	}

	gc.draw(m_text, getX(), getY(), getWidth(), getHeight());

	if(hasFocus()) {
		// draw cursor
	}
}

