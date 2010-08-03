#include "instrumentgraph.hh"
#include "i18n.hh"
#include "screen.hh"

//const unsigned InstrumentGraph::max_panels = 10; // Maximum number of arrow lines / guitar frets


void InstrumentGraph::setupPauseMenu(Menu& menu) {
	menu.clear();
	menu.add(MenuOption(_("Resume"), _("Back to performing!")));
	menu.add(MenuOption(_("Restart"), _("Start the song\nfrom the beginning"), "Sing"));
	menu.add(MenuOption(_("Quit"), _("Exit to song browser"), "Songs"));
}


InstrumentGraph::InstrumentGraph(Audio& audio, Song const& song, input::DevType inp):
  m_audio(audio), m_song(song), m_input(input::DevType(inp)),
  m_stream(),
  m_cx(0.0, 0.2), m_width(0.5, 0.4),
  m_menu(),
  m_button(getThemePath("button.svg")),
  m_text(getThemePath("sing_timetxt.svg"), config["graphic/text_lod"].f()),
  m_selectedTrack(""),
  m_selectedDifficulty(0),
  m_pads(),
  m_correctness(0.0, 5.0),
  m_score(),
  m_scoreFactor(),
  m_starmeter(),
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
}


void InstrumentGraph::doUpdates() {
	if (!menuOpen() && !m_ready) {
		m_ready = true;
		setupPauseMenu(m_menu);
	}
}


void InstrumentGraph::toggleMenu(int forcestate) {
	if (forcestate == 1) { m_menu.open(); return; }
	else if (forcestate == 0) { m_menu.close(); return; }
	m_menu.toggle();
}


void InstrumentGraph::drawMenu() {
	if (m_menu.empty()) return;
	Dimensions dimensions(1.0); // FIXME: bogus aspect ratio (is this fixable?)
	if (getGraphType() == input::DANCEPAD) dimensions.screenTop().middle(m_cx.get()).stretch(m_width.get(), 1.0);
	else dimensions.screenBottom().middle(m_cx.get()).fixedWidth(std::min(m_width.get(),0.5));
	// Some helper vars
	ThemeInstrumentMenu& th = *m_menuTheme;
	MenuOptions::const_iterator cur = static_cast<MenuOptions::const_iterator>(&m_menu.current());
	double w = m_menu.dimensions.w();
	const double offsetX = 0.5f * (dimensions.x1() + dimensions.x2());
	const float txth = th.option.h();
	const float button_margin = 0.05f;
	const float step = txth * 0.7f;
	const float h = m_menu.getOptions().size() * step + step;
	float y = -h * .5f + step;
	float x = -w * .5f + step + offsetX + button_margin * 1.5f;
	// Background
	th.bg.dimensions.middle(offsetX).center(0).stretch(w, h);
	th.bg.draw();
	// Loop through menu items
	w = 0;
	int i = 0;
	for (MenuOptions::const_iterator it = m_menu.begin(); it != m_menu.end(); ++it, ++i) {
		SvgTxtTheme* txt = &th.option;
		// Draw the key hints
		if (getGraphType() != input::DANCEPAD && i < m_pads) {
			int fret = (getGraphType() == input::DRUMS ? ((i + 1) % m_pads) : i);
			glColor4fv(color(fret));
			m_button.dimensions.middle(x - button_margin).center(y).stretch(0.05, 0.05);
			m_button.draw();
		}
		// Selected item?
		if (cur == it) {
			//th.back_h.dimensions.middle(0.05 + offsetX).center(y);
			//th.back_h.draw();
			txt = &th.option_selected;
		}
		txt->dimensions.middle(x).center(y);
		txt->draw(it->getName());
		w = std::max(w, txt->w() + 2 * step + button_margin * 2); // Calculate the widest entry
		y += step;
	}
	// Draw comment text
	if (cur->getComment() != "") {
		//th.comment_bg.dimensions.middle().screenBottom(-0.2);
		//th.comment_bg.draw();
		th.comment.dimensions.middle(offsetX).screenBottom(-0.12);
		th.comment.draw(cur->getComment());
	}
	m_button.dimensions.stretch(1.0, 1.0);
	// Save the calculated menu dimensions
	m_menu.dimensions.stretch(w, h);
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


glutil::Color const& InstrumentGraph::color(int fret) const {
	static glutil::Color fretColors[5] = {
		glutil::Color(0.0f, 0.9f, 0.0f),
		glutil::Color(0.9f, 0.0f, 0.0f),
		glutil::Color(0.9f, 0.9f, 0.0f),
		glutil::Color(0.0f, 0.0f, 1.0f),
		glutil::Color(0.9f, 0.4f, 0.0f)
	};
	if (fret < 0 || fret >= m_pads) throw std::logic_error("Invalid fret number in InstrumentGraph::color");
	if (getGraphType() == input::DRUMS) {
		if (fret == 0) fret = 4;
		else if (fret == 4) fret = 0;
	}
	return fretColors[fret];
}
