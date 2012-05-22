#pragma once

#include <map>
#include <boost/scoped_ptr.hpp>
#include "screen.hh"
#include "glutil.hh"

class Audio;
class ThemeDownloads;

/// options dialogue
class ScreenDownloads: public Screen {
  public:
	/// constructor
	ScreenDownloads(std::string const& name, Audio& audio);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();

  private:
	Audio& m_audio;
	boost::scoped_ptr<ThemeDownloads> m_theme;
	unsigned int m_selected_column;
	boost::scoped_ptr<Surface> m_selector;
	boost::scoped_ptr<Surface> m_mic_icon;
	boost::scoped_ptr<Surface> m_pdev_icon;
};

