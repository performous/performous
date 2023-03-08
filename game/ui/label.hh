#pragma once

#include "control.hh"
#include "text.hh"

class Label : public Control {
  public:
	Label(std::string const& text = {}, Control* parent = nullptr);
	Label(Control* parent, std::string const& text = {});

	void setText(std::string const& text);
	std::string getText() const;

	bool canFocused() const override { return false; }
	void draw(GraphicContext&) override;

	operator Text&() { return *this; }

  private:
	Text m_text;
};
