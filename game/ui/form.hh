#pragma once

#include "usercontrol.hh"

class Form : public UserControl {
  public:
	  void focusNext();
	  void focusPrevious();

	  void onKey(Key) override;

  private:
	  Control* m_focus = nullptr;
};

