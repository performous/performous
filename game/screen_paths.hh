#pragma once

#include <map>
#include <boost/scoped_ptr.hpp>
#include "screen.hh"
#include "textinput.hh"

class Audio;
class ThemeAudioDevices;

/// options dialogue
class ScreenPaths: public Screen {
  public:
	/// constructor
	ScreenPaths(std::string const& name, Audio& m_audio);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void manageEvent(input::NavEvent const& event);
	void draw();

  private:
	Audio& m_audio;
	boost::scoped_ptr<ThemeAudioDevices> m_theme;
	TextInput m_txtinp;
};

