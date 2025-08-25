#pragma once

#include "control.hh"
#include "../texture.hh"
#include "text.hh"

#include <functional>
#include <string>

class TextBox : public Control {
  public:
	TextBox(std::string const& text = {}, Control* parent = nullptr);
	TextBox(Control* parent, std::string const& text = {});

	TextBox& setText(std::string const&, bool keepCursorPosition = false);
	std::string getText() const;
	TextBox& setMaxLength(size_t);
	size_t getMaxLength() const;
	void onTextChanged(std::function<void(TextBox&, std::string const&, std::string const&)> const&);

	void onKey(Key) override;

	void draw(GraphicContext&) override;

  private:
	Text m_text;
	std::unique_ptr<Texture> m_background;
	std::unique_ptr<Texture> m_cursor;
	size_t m_cursorPosition = 0;
	size_t m_maxLength = size_t(-1);
	std::function<void(TextBox&, std::string const&, std::string const&)> m_onTextChanged;
};

