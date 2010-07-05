#pragma once

#include <boost/scoped_ptr.hpp>
#include "audio.hh"
#include "screen.hh"
#include "theme.hh"
//#include "opengl_text.hh"
#include "progressbar.hh"
#include "joystick.hh"

/// screen for practice mode
class ScreenPractice : public Screen {
  public:
	/// constructor
	ScreenPractice(std::string const& name, Audio& audio);
	void enter();
	void exit();
	void manageEvent( SDL_Event event );
	void draw();

	/// draw analyzers
	void draw_analyzers();

  private:
	Audio& m_audio;
	std::vector<std::string> m_samples;
	boost::ptr_vector<ProgressBar> m_vumeters;
	boost::scoped_ptr<ThemePractice> theme;
};
