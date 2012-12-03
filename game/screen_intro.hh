#pragma once

#include <boost/scoped_ptr.hpp>
#include "screen.hh"
#include "menu.hh"
#include "animvalue.hh"

class Audio;
class ThemeIntro;
class SvgTxtTheme;
class MenuOption;

/// intro screen
class ScreenIntro : public Screen {
  public:
	/// constructor
	ScreenIntro(std::string const& name, Audio& audio);
	void enter();
	void exit();
	void reloadGL();
	void manageEvent(SDL_Event event);
	void manageEvent(input::NavEvent const& event);
	void draw();

	/// draw menu
	void draw_menu_options();

  private:
	void populateMenu();
	SvgTxtTheme& getTextObject(std::string const& txt);

	Audio& m_audio;
	boost::scoped_ptr<ThemeIntro> theme;
	Menu m_menu;
	bool m_first;
	AnimValue m_selAnim;
	AnimValue m_submenuAnim;
};
