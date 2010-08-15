#pragma once

#include <boost/scoped_ptr.hpp>
#include "dialog.hh"
#include "screen.hh"
#include "menu.hh"

class Audio;
class ThemeIntro;
class MenuOption;

/// intro screen
class ScreenIntro : public Screen {
  public:
	/// constructor
	ScreenIntro(std::string const& name, Audio& audio);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();

	/// draw menu
	void draw_menu_options();

  private:
	Audio& m_audio;
	boost::scoped_ptr<ThemeIntro> theme;
	boost::scoped_ptr<Dialog> m_dialog;
	Menu m_menu;
	bool m_first;
};
