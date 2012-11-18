#include "screen_practice.hh"

#include "audio.hh"
#include "util.hh"
#include "fs.hh"
#include "controllers.hh"
#include "theme.hh"
#include "progressbar.hh"

ScreenPractice::ScreenPractice(std::string const& name, Audio& audio):
  Screen(name), m_audio(audio)
{}

void ScreenPractice::enter() {
	m_audio.playMusic(getThemePath("practice.ogg"));
	// draw vu meters
	for (unsigned int i = 0, mics = m_audio.analyzers().size(); i < mics; ++i) {
		m_vumeters.push_back(new ProgressBar(getThemePath("vumeter_bg.svg"), getThemePath("vumeter_fg.svg"), ProgressBar::VERTICAL, 0.136, 0.023));
	}
	m_samples.push_back("drum bass");
	m_samples.push_back("drum snare");
	m_samples.push_back("drum hi-hat");
	m_samples.push_back("drum tom1");
	m_samples.push_back("drum cymbal");
	//m_samples.push_back("drum tom2");
	reloadGL();
}

void ScreenPractice::reloadGL() {
	theme.reset(new ThemePractice());
}

void ScreenPractice::exit() {
	m_vumeters.clear();
	m_samples.clear();
	theme.reset();
}

void ScreenPractice::manageEvent(SDL_Event event) {
	ScreenManager * sm = ScreenManager::getSingletonPtr();
	input::NavButton nav(input::getNav(event));
	if (nav == input::CANCEL || nav == input::START || nav == input::SELECT) sm->activateScreen("Intro");
	else if (nav == input::PAUSE) m_audio.togglePause();
	// FIXME: This should not use stuff from input::detail namespace!
	else if (event.type == SDL_JOYBUTTONDOWN // Play drum sounds here
	  && input::detail::devices.find(event.jbutton.which)->second.type_match(input::DRUMS)) {
		int b = input::detail::devices.find(event.jbutton.which)->second.buttonFromSDL(event.jbutton.button);
		if (b != -1) m_audio.playSample(m_samples[unsigned(b) % m_samples.size()]);
	}
}

void ScreenPractice::draw() {
	theme->bg.draw();
	this->draw_analyzers();
}

void ScreenPractice::draw_analyzers() {
	theme->note.dimensions.fixedHeight(0.03f);
	theme->sharp.dimensions.fixedHeight(0.09f);
	MusicalScale scale;
	boost::ptr_vector<Analyzer>& analyzers = m_audio.analyzers();
	if (analyzers.empty()) return;
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
		m_vumeters[i].dimensions.screenBottom().left(-0.4 + i * 0.2).fixedWidth(0.04);
		m_vumeters[i].draw(analyzer.getPeak() / 43.0 + 1.0);

		if (freq != 0.0) {
			Analyzer::tones_t tones = analyzer.getTones();

			for (Analyzer::tones_t::const_iterator t = tones.begin(); t != tones.end(); ++t) {
				if (t->age < Tone::MINAGE) continue;
				int note = scale.getNoteId(t->freq);
				if (note < 0) continue;
				int octave = note / 12 - 1;
				double noteOffset = scale.getNoteNum(note);
				bool sharp = scale.isSharp(note);
				noteOffset += (octave - 3) * 7;
				noteOffset += 0.4 * scale.getNoteOffset(t->freq);
				float posXnote = -0.25 + 0.2 * i + 0.002 * t->stabledb;
				float posYnote = .075-noteOffset*0.015;

				theme->note.dimensions.left(posXnote).center(posYnote);
				theme->note.draw();
				if (sharp) {
					theme->sharp.dimensions.right(posXnote).center(posYnote);
					theme->sharp.draw();
				}
			}
		}
	}
	// Display note and frequency
	if (textFreq > 0.0) theme->note_txt.draw(scale.getNoteStr(textFreq));
}
