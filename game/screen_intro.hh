#pragma once

#include "dialog.hh"
#include "screen.hh"
#include "theme.hh"
#include "menu.hh"
#include <boost/scoped_ptr.hpp>

class Audio;
class Capture;

/// intro screen
class ScreenIntro : public Screen {
  public:
	/// constructor
	ScreenIntro(std::string const& name, Audio& audio, Capture& capture);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();

  private:
	Audio& m_audio;
	Capture& m_capture;
	boost::scoped_ptr<ThemeIntro> theme;
	boost::ptr_vector<MenuOption> m_menuOptions;
	boost::scoped_ptr<Dialog> m_dialog;
	unsigned int selected;
};
