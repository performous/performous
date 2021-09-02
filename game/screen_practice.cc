#include "screen_practice.hh"

#include "audio.hh"
#include "util.hh"
#include "fs.hh"
#include "controllers.hh"
#include "theme.hh"
#include "progressbar.hh"
#include "game.hh"

ScreenPractice::ScreenPractice(std::string const& name, Audio& audio):
  Screen(name), m_audio(audio)
{}

void ScreenPractice::enter() {
	m_audio.playMusic(findFile("practice.ogg"));
	// draw vu meters
	for (unsigned int i = 0, mics = m_audio.analyzers().size(); i < mics; ++i) {
		auto progressBarPtr = std::unique_ptr<ProgressBar>(std::make_unique<ProgressBar>(findFile("vumeter_bg.svg"), findFile("vumeter_fg.svg"), ProgressBar::VERTICAL, 0.136, 0.023));
		m_vumeters.push_back(std::move(progressBarPtr));
	}
	m_samples.push_back("drum bass");
	m_samples.push_back("drum snare");
	m_samples.push_back("drum hi-hat");
	m_samples.push_back("drum tom1");
	m_samples.push_back("drum cymbal");
	//m_samples.push_back("drum tom2");
	reloadGL();
	Game::getSingletonPtr()->controllers.enableEvents(true);
}

void ScreenPractice::reloadGL() {
	theme = std::make_unique<ThemePractice>();
}

void ScreenPractice::exit() {
	Game::getSingletonPtr()->controllers.enableEvents(false);
	m_vumeters.clear();
	m_samples.clear();
	theme.reset();
}

void ScreenPractice::manageEvent(input::NavEvent const& event) {
	Game* gm = Game::getSingletonPtr();
	input::NavButton nav = event.button;
	if (nav == input::NAV_CANCEL || nav == input::NAV_START) gm->activateScreen("Intro");
	else if (nav == input::NAV_PAUSE) m_audio.togglePause();
	// Process all instrument events that are available, then throw away the instruments...
	input::DevicePtr dev = gm->controllers.registerDevice(event.source);
	if (dev) {
		for (input::Event ev; dev->getEvent(ev);) {
			if (ev.value == 0.0) continue;
			if (dev->type == input::DEVTYPE_DANCEPAD) {}
			else if (dev->type == input::DEVTYPE_GUITAR) {}
			else if (dev->type == input::DEVTYPE_DRUMS) m_audio.playSample(m_samples[ev.button.num() % m_samples.size()]);
		}
	}
	// TODO: We could store the DevicePtrs and display the instruments on screen in a meaningful way
	// Note: Alternatively this could be done via listening to NavEvents and not even registering the devices, simplifying the above processing.
}

void ScreenPractice::draw() {
	theme->bg.draw();
	this->draw_analyzers();
}

void ScreenPractice::draw_analyzers() {
	theme->note.dimensions.fixedHeight(0.03f);
	theme->sharp.dimensions.fixedHeight(0.09f);
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
		m_vumeters[i]->dimensions.screenBottom().left(-0.4 + i * 0.08).fixedWidth(0.04); //0.08 was originally 0.2. Now 11 in a row fits
		m_vumeters[i]->draw(analyzer.getPeak() / 43.0 + 1.0);

		if (freq != 0.0) {
			Analyzer::tones_t tones = analyzer.getTones();

			for (Analyzer::tones_t::const_iterator t = tones.begin(); t != tones.end(); ++t) {
				if (t->age < 3) continue;
				if (!scale.setFreq(t->freq).isValid()) continue;
				double line = scale.getNoteLine() + 0.4 * scale.getNoteOffset();
				float posXnote = -0.25 + 0.2 * i + 0.002 * t->stabledb;  // Wiggle horizontally based on volume
				float posYnote = -0.03 - line * 0.015;  // On treble key (C4), plus offset (lines)

				theme->note.dimensions.left(posXnote).center(posYnote);
				theme->note.draw();
				// Draw # for sharp notes
				if (scale.isSharp()) {
					theme->sharp.dimensions.right(posXnote).center(posYnote);
					theme->sharp.draw();
				}
			}
		}
	}
	// Display note and frequency
	if (textFreq > 0.0) theme->note_txt.draw(scale.setFreq(textFreq).getStr());
}
