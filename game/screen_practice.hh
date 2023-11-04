#pragma once

#include "controllers.hh"
#include "screen.hh"
#include "theme/theme.hh"

class Audio;
class Sample;
class ProgressBar;
class ThemePractice;

/// screen for practice mode
class ScreenPractice : public Screen {
  public:
	/// constructor
	ScreenPractice(Game &game, std::string const& name, Audio& audio);
	void enter();
	void exit();
	void reloadGL();
	void manageEvent(input::NavEvent const& event);
	void draw();

	/// draw analyzers
	void draw_analyzers();

  private:
	Audio& m_audio;
	std::vector<std::string> m_samples;
	std::vector<std::unique_ptr<ProgressBar>> m_vumeters;
	std::shared_ptr<ThemePractice> m_theme;
};
