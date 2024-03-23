#pragma once

#include "screen.hh"
#include "theme.hh"

class Audio;
class Sample;
class ProgressBar;

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
	std::vector<std::vector<std::unique_ptr<Texture>>> m_bars;
	std::unique_ptr<ThemeGuitarTuner> m_theme;
};
