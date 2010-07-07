#include "instrumentgraph.hh"

#include "screen.hh"

//const unsigned InstrumentGraph::max_panels = 10; // Maximum number of arrow lines / guitar frets

InstrumentGraph::InstrumentGraph(Audio& audio, Song const& song, input::DevType inp):
  m_audio(audio), m_song(song), m_input(input::DevType(inp)),
  m_stream(),
  m_cx(0.0, 0.2), m_width(0.5, 0.4),
  m_menu(),
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
	m_menuTheme.reset(new ThemeInstrumentMenu());

	// Populate joining menu
	m_menu.add(MenuOption(_("Ready!"), _("Start performing!")));
	// Guitar- / dancegraph specific options can be added in their constructors
}


void InstrumentGraph::setupPauseMenu() {
	m_menu.clear();
	// TODO: Replace with options that actually do something
	m_menu.add(MenuOption(_("Resume"), _("Back to performing!")));
	//m_menu.add(MenuOption(_("Pause"), _("Toggle pause"), &InstrumentGraph::togglePause));
	m_menu.add(MenuOption(_("Restart"), _("Start the song from the beginning")));
	m_menu.add(MenuOption(_("Quit"), _("Exit to song browser"), "Songs"));
}


void InstrumentGraph::doUpdates() {
	if (!menuOpen() && !m_ready) {
		m_ready = true;
		setupPauseMenu();
	}
}


void InstrumentGraph::toggleMenu(bool forceopen) {
	if (forceopen) { m_menu.open(); return; }
	m_menu.toggle();
}


void InstrumentGraph::drawMenu(double offsetX) {
	if (m_menu.empty()) return;
	float step = 0.075;
	float y = -0.5 * m_menu.getOptions().size() * step;
	// Some helper vars
	ThemeInstrumentMenu& th = *m_menuTheme;
	MenuOptions::const_iterator cur = m_menu.current();
	// Loop through menu items
	for (MenuOptions::const_iterator it = m_menu.begin(); it != m_menu.end(); ++it) {
		SvgTxtTheme* txt = &th.option;
		if (cur == it) {
			//th.back_h.dimensions.middle(0.05 + offsetX).center(y);
			//th.back_h.draw();
			txt = &th.option_selected;
		}
		txt->dimensions.middle(-0.1 + offsetX).center(y);
		txt->draw(it->name);

		y += step;
	}
	if (cur->comment != "") {
		//th.comment_bg.dimensions.middle().screenBottom(-0.2);
		//th.comment_bg.draw();
		th.comment.dimensions.middle(-0.1 + offsetX).screenBottom(-0.2);
		th.comment.draw(cur->comment);
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
