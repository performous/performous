#include "screen_guitar_tuner.hh"

#include "audio.hh"
#include "util.hh"
#include "fs.hh"
#include "controllers.hh"
#include "theme.hh"
#include "progressbar.hh"
#include "game.hh"
#include "analyzer.hh"



ScreenGuitarTuner::ScreenGuitarTuner(Game& game, std::string const& name, Audio& audio)
	: Screen(game, name), m_audio(audio) {
}

void ScreenGuitarTuner::enter() {
	for (size_t i = 0, mics = m_audio.analyzers().size(); i < mics; ++i) {
		m_vumeters.emplace_back(std::make_unique<ProgressBar>(findFile("vumeter_bg.svg"), findFile("vumeter_fg.svg"), ProgressBar::Mode::VERTICAL, 0.136, 0.023));
	}

	reloadGL();
	getGame().controllers.enableEvents(true);
}

void ScreenGuitarTuner::reloadGL() {
	m_theme = std::make_unique<ThemeGuitarTuner>();
}

void ScreenGuitarTuner::exit() {
	getGame().controllers.enableEvents(false);
	m_vumeters.clear();
	m_theme.reset();
}

void ScreenGuitarTuner::manageEvent(input::NavEvent const& event) {
	auto const nav = event.button;
	if (nav == input::NavButton::CANCEL || nav == input::NavButton::START)
		getGame().activateScreen("Intro");

	auto dev = getGame().controllers.registerDevice(event.source);
	if (dev) {
		for (input::Event ev; dev->getEvent(ev);) {
			if (ev.value == 0.0)
				continue;
		}
	}
}

void ScreenGuitarTuner::draw() {
	auto& window = getGame().getWindow();

	m_theme->bg.draw(window);
	m_theme->fret.dimensions.left(-0.25f).top(-0.15f).stretch(0.5f, 0.3f);
	m_theme->fret.draw(window);

	draw_analyzers();
}

void ScreenGuitarTuner::draw_analyzers() {
	auto& window = getGame().getWindow();

	//m_theme->note.dimensions.fixedHeight(0.03f);
	//m_theme->sharp.dimensions.fixedHeight(0.09f);

	auto& analyzers = m_audio.analyzers();

	if (analyzers.empty())
		return;

	MusicalScale scale;
	double textPower = -getInf();
	double textFreq = 0.0;

	for (unsigned int i = 0; i < analyzers.size(); ++i) {
		auto& analyzer = analyzers[i];

		analyzer.process();

		auto const tone = analyzer.findTone();
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
			auto tones = analyzer.getTones();

			for (auto t = tones.begin(); t != tones.end(); ++t) {
				if (t->age < Tone::MINAGE)
					continue;
				if (!scale.setFreq(t->freq).isValid())
					continue;

				double line = scale.getNoteLine() + 0.4 * scale.getNoteOffset();
				float posXnote = static_cast<float>(-0.25 + 0.2 * i + 0.002 * t->stabledb);  // Wiggle horizontally based on volume
				float posYnote = static_cast<float>(-0.03 - line * 0.015);  // On treble key (C4), plus offset (lines)

				//m_theme->note.dimensions.left(posXnote).center(posYnote);
				//m_theme->note.draw(window);
				// Draw # for sharp notes
				if (scale.isSharp()) {
					//m_theme->sharp.dimensions.right(posXnote).center(posYnote);
					//m_theme->sharp.draw(window);
				}
			}
		}
	}

	if (textFreq > 0.0)
		m_theme->note_txt.draw(window, scale.setFreq(textFreq).getStr());
}
