#pragma once

#include "control.hh"
#include "text.hh"

class Button : public Control {
  public:
	Button(std::string const& text = {}, Control* parent = nullptr);
	Button(Control* parent, std::string const& text = {});

	void setText(std::string const& text);
	std::string getText() const;

	void onClicked(std::function<void(Button&)> const&);

	void onKey(Key) override;

	bool canFocused() const override { return true; }
	void draw(GraphicContext&) override;

  private:
	Text m_text;
	Texture m_background;
	std::function<void(Button&)> m_onClicked;
};

