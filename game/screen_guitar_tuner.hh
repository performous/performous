#pragma once

#include "screen.hh"
#include "theme.hh"

class Audio;
class Sample;
class ProgressBar;
class ThemePractice;

class ScreenGuitarTuner : public Screen {
public:
	ScreenGuitarTuner(Game& game, std::string const& name, Audio& audio);

	void enter();
	void exit();
	void reloadGL();
	void manageEvent(input::NavEvent const& event);
	void draw();

	void draw_analyzers();

private:
	Audio& m_audio;
	std::vector<std::string> m_samples;
	std::vector<std::unique_ptr<ProgressBar>> m_vumeters;
	std::unique_ptr<ThemePractice> theme;
};
