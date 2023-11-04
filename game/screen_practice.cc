#include "screen_practice.hh"

#include "audio.hh"
#include "util.hh"
#include "fs.hh"
#include "controllers.hh"
#include "theme/theme.hh"
#include "theme/theme_loader.hh"
#include "progressbar.hh"
#include "game.hh"
#include "analyzer.hh"
#include "graphic/color_trans.hh"

#include <SDL_timer.h>

ScreenPractice::ScreenPractice(Game &game, std::string const& name, Audio& audio):
  Screen(game, name), m_audio(audio)
{}

void ScreenPractice::enter() {
	m_audio.playMusic(getGame(), findFile("practice.ogg"));
	// draw vu meters
	for (size_t i = 0, mics = m_audio.analyzers().size(); i < mics; ++i) {
		auto progressBarPtr = std::unique_ptr<ProgressBar>(std::make_unique<ProgressBar>(findFile("vumeter_bg.svg"), findFile("vumeter_fg.svg"), ProgressBar::Mode::VERTICAL, 0.136, 0.023));
		m_vumeters.push_back(std::move(progressBarPtr));
	}
	m_samples.push_back("drum bass");
	m_samples.push_back("drum snare");
	m_samples.push_back("drum hi-hat");
	m_samples.push_back("drum tom1");
	m_samples.push_back("drum cymbal");
	//m_samples.push_back("drum tom2");
	reloadGL();
	getGame().controllers.enableEvents(true);
}

void ScreenPractice::reloadGL() {
	auto loader = ThemeLoader();

	m_theme = loader.load<ThemePractice>(getName());

	if (!m_theme)
		m_theme = std::make_unique<ThemePractice>();
}

void ScreenPractice::exit() {
	getGame().controllers.enableEvents(false);
	m_vumeters.clear();
	m_samples.clear();
	m_theme.reset();
}

void ScreenPractice::manageEvent(input::NavEvent const& event) {
	input::NavButton nav = event.button;
	if (nav == input::NavButton::CANCEL || nav == input::NavButton::START) getGame().activateScreen("Intro");
	else if (nav == input::NavButton::PAUSE) m_audio.togglePause();
	// Process all instrument events that are available, then throw away the instruments...
	input::DevicePtr dev = getGame().controllers.registerDevice(event.source);
	if (dev) {
		for (input::Event ev; dev->getEvent(ev);) {
			if (ev.value == 0.0) continue;
			if (dev->type == input::DevType::DANCEPAD) {}
			else if (dev->type == input::DevType::GUITAR) {}
			else if (dev->type == input::DevType::DRUMS) m_audio.playSample(m_samples[ev.button.num() % m_samples.size()]);
		}
	}
	// TODO: We could store the DevicePtrs and display the instruments on screen in a meaningful way
	// Note: Alternatively this could be done via listening to NavEvents and not even registering the devices, simplifying the above processing.
}

void ScreenPractice::draw() {
	auto& window = getGame().getWindow();
	{
		if (m_theme->colorcycling) {
			auto const cycleDurationMS = m_theme->colorcycleduration * 1000;
			auto anim = static_cast<float>(SDL_GetTicks() % cycleDurationMS) / float(cycleDurationMS);
			auto c = ColorTrans(window, glmath::rotate(static_cast<float>(TAU * anim), glmath::vec3(1.0f, 1.0f, 1.0f)));

			m_theme->bg.draw(window);
		}
		else {
			m_theme->bg.draw(window);
		}
	}
	this->draw_analyzers();
}

void ScreenPractice::draw_analyzers() {
	auto& window = getGame().getWindow();
	m_theme->note.dimensions.fixedHeight(0.03f);
	m_theme->sharp.dimensions.fixedHeight(0.09f);
	auto& analyzers = m_audio.analyzers();
	if (analyzers.empty()) return;
	MusicalScale scale;
	double textPower = -getInf();
	double textFreq = 0.0;

	for (unsigned int i = 0; i < analyzers.size(); ++i) {
		Analyzer& analyzer = analyzers[i];
		analyzer.process();
		Tone const* tone = analyzer.findTone();
		double freq = (tone ? tone->freq : 0.0);
		if (tone && tone->db > textPower) {
			textPower = tone->db;
			textFreq = freq;
		}
		// getPeak returns 0.0 when clipping, negative values when not that loud.
		// Normalizing to [0,1], where 0 is -43 dB or less (to match the vumeter graphic)
		m_vumeters[i]->dimensions.screenBottom().left(-0.4f + static_cast<float>(i) * 0.08f).fixedWidth(0.04f); //0.08 was originally 0.2. Now 11 in a row fits
		m_vumeters[i]->draw(window, static_cast<float>(analyzer.getPeak() / 43.0 + 1.0));

		if (freq != 0.0) {
			Analyzer::tones_t tones = analyzer.getTones();

			for (Analyzer::tones_t::const_iterator t = tones.begin(); t != tones.end(); ++t) {
				if (t->age < Tone::MINAGE) continue;
				if (!scale.setFreq(t->freq).isValid()) continue;
				double line = scale.getNoteLine() + 0.4 * scale.getNoteOffset();
				float posXnote = static_cast<float>(-0.25 + 0.2 * i + 0.002 * t->stabledb);  // Wiggle horizontally based on volume
				float posYnote = static_cast<float>(-0.03 - line * 0.015);  // On treble key (C4), plus offset (lines)

				m_theme->note.dimensions.left(posXnote).center(posYnote);
				m_theme->note.draw(window);
				// Draw # for sharp notes
				if (scale.isSharp()) {
					m_theme->sharp.dimensions.right(posXnote).center(posYnote);
					m_theme->sharp.draw(window);
				}
			}
		}
	}
	// Display note and frequency
	if (textFreq > 0.0)
		m_theme->note_txt.draw(window, scale.setFreq(textFreq).getStr());
}
