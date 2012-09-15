#pragma once

#include <map>
#include <boost/scoped_ptr.hpp>
#include "screen.hh"
#include "glutil.hh"

class Audio;
class ThemeDownloads;
class Downloader;

/// options dialogue
class ScreenDownloads: public Screen {
  public:
	/// constructor
	ScreenDownloads(std::string const& name, Audio& audio, Downloader &downloader);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();
  private:
	Audio& m_audio;
	Downloader& m_downloader;
	boost::scoped_ptr<ThemeDownloads> m_theme;
	unsigned int m_selectedTorrent;
};

