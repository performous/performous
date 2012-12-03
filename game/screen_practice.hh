#pragma once

#include <boost/scoped_ptr.hpp>
#include "screen.hh"
#include "controllers.hh"

class Audio;
class Sample;
class ProgressBar;
class ThemePractice;

/// screen for practice mode
class ScreenPractice : public Screen {
  public:
	/// constructor
	ScreenPractice(std::string const& name, Audio& audio);
	void enter();
	void exit();
	void reloadGL();
	void manageEvent( SDL_Event event );
	void manageEvent(input::NavEvent const& event);
	void draw();

	/// draw analyzers
	void draw_analyzers();

  private:
	Audio& m_audio;
	std::vector<std::string> m_samples;
	boost::ptr_vector<ProgressBar> m_vumeters;
	boost::scoped_ptr<ThemePractice> theme;
};
