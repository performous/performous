#include "instrumentgraph.hh"

#include "screen.hh"

//const unsigned InstrumentGraph::max_panels = 10; // Maximum number of arrow lines / guitar frets

InstrumentGraph::InstrumentGraph(Audio& audio, Song const& song, input::DevType inp):
  m_audio(audio), m_song(song), m_input(input::DevType(inp)),
  m_stream(),
  m_cx(0.0, 0.2), m_width(0.5, 0.4),
  m_menu(*this),
  m_menuOpen(true),
  m_text(getThemePath("sing_timetxt.svg"), config["graphic/text_lod"].f()),
  m_pads(),
  m_correctness(0.0, 5.0),
  m_score(),
  m_scoreFactor(),
  m_streak(),
  m_longestStreak(),
  m_bigStreak(),
  m_countdown(3), // Display countdown 3 secs before note start
  m_jointime(getNaN()),
  m_dead(),
  m_ready()
{
	m_popupText.reset(new SvgTxtThemeSimple(getThemePath("sing_popup_text.svg"), config["graphic/text_lod"].f()));

	// Populate joining menu
	m_menu.add(InstrumentMenuOption(_("Ready!"), _("Start performing!"), &InstrumentGraph::toggleMenu));
	// Guitar- / dancegraph specific options can be added in their constructors
}


void InstrumentGraph::setupPauseMenu() {
	m_menu.clear();
	m_menu.add(InstrumentMenuOption(_("Resume"), _("Back to performing!"), &InstrumentGraph::toggleMenu));
	//m_menu.add(InstrumentMenuOption(_("Pause"), _("Toggle pause"), &InstrumentGraph::togglePause));
	m_menu.add(InstrumentMenuOption(_("Restart"), _("Start the song from the beginning"), &InstrumentGraph::restart));
	m_menu.add(InstrumentMenuOption(_("Quit"), _("Exit to song browser"), &InstrumentGraph::quit));
}


void InstrumentGraph::toggleMenu(int dontforce) {
	if (dontforce == 0) { m_menuOpen = true; return; }
	if (m_menuOpen && !m_ready) {
		m_ready = true;
		setupPauseMenu();
	}
	m_menuOpen = !m_menuOpen;
}


void InstrumentGraph::drawMenu(double offsetX) {
	if (m_menu.options.empty()) return;
	float step = 0.05;
	float y = -0.5 * m_menu.options.size() * step;
	m_text.dimensions.screenCenter(0).middle(0);
	for (InstrumentMenuOptions::iterator it = m_menu.options.begin(); it != m_menu.options.end(); ++it) {
		// TODO: Use theme
		m_text.dimensions.center(y).middle(-0.09 + offsetX);
		std::string value = it->value;
		if (m_menu.current == it) value = ">  " + value + "  <";
		m_text.draw(value);
		y += step;
	}
	if (m_menu.current->comment != "") {
		m_text.dimensions.center(y+3*step).middle(-0.09 + offsetX);
		m_text.draw(m_menu.current->comment);
	}
}


void InstrumentGraph::drawPopups(double offsetX) {
	for (Popups::iterator it = m_popups.begin(); it != m_popups.end(); ) {
		if (!it->draw(offsetX)) { it = m_popups.erase(it); continue; }
		++it;
	}
}


void InstrumentGraph::handleCountdown(double time, double beginTime) {
	if (!dead() && time < beginTime && time >= beginTime - m_countdown - 1) {
		m_popups.push_back(Popup(m_countdown > 0 ?
		  std::string("- ") +boost::lexical_cast<std::string>(unsigned(m_countdown))+" -" : "Rock On!",
		  glutil::Color(0.0f, 0.0f, 1.0f), 2.0, m_popupText.get()));
		  --m_countdown;
	}
}
