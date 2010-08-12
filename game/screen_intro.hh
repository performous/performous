#pragma once

#include <boost/scoped_ptr.hpp>
#include "dialog.hh"
#include "screen.hh"

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
	boost::ptr_vector<MenuOption> m_menuOptions;
	boost::scoped_ptr<Dialog> m_dialog;
	unsigned int selected;
	bool m_first;
};
