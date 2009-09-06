#pragma once

#include "screen.hh"
#include "audio.hh"
#include "theme.hh"
#include <boost/scoped_ptr.hpp>

/// options dialogue
class ScreenConfiguration: public Screen {
  public:
	/// constructor
	ScreenConfiguration(std::string const& name, Audio& m_audio);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();

  private:
	Audio& m_audio;
	boost::scoped_ptr<ThemeConfiguration> theme;
	std::vector<std::string> configuration;
	unsigned int selected;
};

