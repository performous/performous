#pragma once

#include "config.hh"
#include "menu.hh"
#include "screen.hh"
#include "songs.hh"
#include "textinput.hh"
#include "theme/theme.hh"
#include "event_manager.hh"
#include "fs.hh"

#include <iostream>
#include <map>
#include <string>

class Audio;
class ThemeAudioDevices;

/// options dialogue
class ScreenPaths: public Screen {
  public:
	/// constructor
	ScreenPaths(Game &game, std::string const& name, Audio& audio, Songs& songs);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void manageEvent(input::NavEvent const& event);
	void draw();
	void generateMenuFromPath(fs::path path);

  private:
	void onEnter(EventParameter const&);

	Audio& m_audio;
	Songs& m_songs;
	std::shared_ptr<ThemePaths> m_theme;
	Menu m_menu;
	AnimValue m_selAnim;
};

