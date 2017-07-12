#pragma once

#include <map>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <string>
#include "menu.hh"
#include "screen.hh"
#include "textinput.hh"
#include "config.hh"
#include "songs.hh"

class Audio;
class ThemeAudioDevices;

/// options dialogue
class ScreenPaths: public Screen {
  public:
	/// constructor
	ScreenPaths(std::string const& name, Audio& audio, Songs& songs);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void manageEvent(input::NavEvent const& event);
	void draw();
	void generateMenuFromPath(fs::path path);

  private:
	Audio& m_audio;
	Songs& m_songs;
	boost::scoped_ptr<ThemeAudioDevices> m_theme;
	Menu m_menu;
	AnimValue m_selAnim;
};

