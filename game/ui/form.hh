#pragma once

#include "usercontrol.hh"

class Form : public UserControl {
  public:
	void focus(Control const&);
	void focusNext();
	void focusPrevious();

	void onKey(Key) override;
	void onKeyDown(Key key) override;
	void onKeyUp(Key key) override;

  private:
	  Control* m_focus = nullptr;
};

