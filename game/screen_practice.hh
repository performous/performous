#pragma once
#ifndef __SCREENPRACTICE_H__
#define __SCREENPRACTICE_H__

#include <boost/scoped_ptr.hpp>
#include "screen.hh"
#include "theme.hh"
#include "opengl_text.hh"
#include "progressbar.hh"
#include "joystick.hh"

/// screen for practice mode
class ScreenPractice : public Screen {
  public:
	/// constructor
	ScreenPractice(std::string const& name, Audio& audio, Capture& capture);
	void enter();
	void exit();
	void manageEvent( SDL_Event event );
	void draw();
	
	/// draw analyzers
	void draw_analyzers();

  private:
	Audio& m_audio;
	Capture& m_capture;
	boost::ptr_vector<ProgressBar> m_vumeters;
	boost::scoped_ptr<ThemePractice> theme;
	boost::scoped_ptr<input::InputDev> drums;
};

#endif
