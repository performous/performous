#pragma once

#include "usercontrol.hh"

class Form : public UserControl {
  public:
	Form(Game&);

	void focus(Control const&);
	void focusNext();
	void focusPrevious();

	void onKey(Key) override;
	void onKeyDown(Key key) override;
	void onKeyUp(Key key) override;

	Game& getGame() override;

  private:
	  Game& m_game;
	  Control* m_focus = nullptr;
};

