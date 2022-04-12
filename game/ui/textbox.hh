#pragma once

#include "control.hh"
#include "../texture.hh"
#include "text.hh"

class TextBox : public Control {
  public:
	TextBox(std::string const& text = {}, Control* parent = nullptr);
	TextBox(Control* parent, std::string const& text = {});

	TextBox& setText(std::string const&);
	std::string getText() const;
	TextBox& setMaxLength(size_t);
	size_t getMaxLength() const;

	void onKey(Key) override;

	void draw(GraphicContext&) override;

  private:
	Text m_text;
	Texture m_background;
	Texture m_cursor;
	size_t m_cursorPosition = 0;
	size_t m_maxLength = size_t(-1);
};

