#include "screen_guitar_tuner.hh"

#include "audio.hh"
#include "util.hh"
#include "fs.hh"
#include "controllers.hh"
#include "theme.hh"
#include "progressbar.hh"
#include "game.hh"
#include "analyzer.hh"
#include <guitar/guitar_strings.hh>



ScreenGuitarTuner::ScreenGuitarTuner(Game& game, std::string const& name, Audio& audio)
	: Screen(game, name), m_audio(audio) {
}

void ScreenGuitarTuner::enter() {
	m_audio.fadeout(getGame());

	for (size_t i = 0, mics = m_audio.analyzers().size(); i < mics; ++i) {
		m_bars.emplace_back();
		for(auto n = 0; n < 7; ++n)
			m_bars[i].emplace_back(std::make_unique<Texture>(findFile("bar.svg")));
	}

	reloadGL();
	getGame().controllers.enableEvents(true);
}

void ScreenGuitarTuner::reloadGL() {
	m_theme = std::make_unique<ThemeGuitarTuner>();
}

void ScreenGuitarTuner::exit() {
	getGame().controllers.enableEvents(false);
	m_bars.clear();
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

	auto const width = m_theme->fretWidth;
	auto const height = m_theme->fretHeight;
	auto const left = width * -0.5f;
	auto const top = height * -0.5f;

	m_theme->bg.draw(window);
	m_theme->fret.dimensions.left(left).top(top).stretch(width, height);
	m_theme->fret.draw(window);

	draw_analyzers();
}

void ScreenGuitarTuner::draw_analyzers() {
	auto& window = getGame().getWindow();
	auto& analyzers = m_audio.analyzers();

	if (analyzers.empty())
		return;

	auto const paddingTop = m_theme->paddingTop;
	auto const paddingBottom = m_theme->paddingBottom;
	auto const height = m_theme->fretHeight - m_theme->fretHeight * (paddingTop + paddingBottom);

	MusicalScale scale;
	auto textPower = -getInf();
	auto textFreq = 0.0;

	for (unsigned int i = 0; i < analyzers.size(); ++i) {
		auto& analyzer = analyzers[i];

		analyzer.process();

		auto const tone = analyzer.findTone();
		double freq = (tone ? tone->freq : 0.0);
		if (tone && tone->db > textPower) {
			textPower = tone->db;
			textFreq = freq;
		}

		if (freq != 0.0) {
			auto const tones = analyzer.getTones();
			auto n = 0U;

			for (auto& tone : tones) {
				if (tone.age < Tone::MINAGE)
					continue;
				//if (!scale.setFreq(tone.freq).isValid())
				//	continue;

				auto const string = GuitarStrings().getString(static_cast<Frequency>(tone.freq));
				auto const base = GuitarStrings().getBaseFrequency(string);
				auto const difference = base - static_cast<Frequency>(tone.freq);
				auto const x = difference / base;
				auto const line = static_cast<float>(static_cast<int>(string)) / 5.f;
				auto const y = height * (-0.5f + line);

				m_bars[i][n]->dimensions.middle(x).center(y).stretch(0.04f, 0.04f /* * static_cast<float>(tone.db) / 40.f*/);
				m_bars[i][n]->draw(window);

				if (++n == m_bars[i].size())
					break;
			}
		}
	}

	if (textFreq > 0.0) {
		auto const string = GuitarStrings().getString(static_cast<Frequency>(textFreq));
		auto const line = static_cast<float>(static_cast<int>(string)) / 5.f;
		auto const y = height * (-0.5f + line);

		m_theme->note_txt.dimensions.center(y).right(m_theme->fretWidth * -0.5f - 0.2f);
		m_theme->note_txt.draw(window, scale.setFreq(textFreq).getStr());
	}
}
