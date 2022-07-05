#pragma once

#include "control.hh"
#include <texture.hh>
#include <memory>

class CheckBox : public Control {
  public:
	CheckBox(bool checked = false, Control* parent = nullptr);
	CheckBox(Control* parent, bool checked = false);

	bool isChecked() const;
	void setChecked(bool);
	bool getState() const;
	void toggleState();


	void onStateChanged(std::function<void(CheckBox&, bool)> const&);

	void onKey(Key) override;

	bool canFocused() const override { return true; }
	void draw(GraphicContext&) override;

  private:
	Texture m_checkedImage;
	Texture m_uncheckedImage;
	bool m_checked;
	std::function<void(CheckBox&, bool)> m_onStateChanged;
};

