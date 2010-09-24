#include "instrumentgraph.hh"
#include "i18n.hh"
#include "screen.hh"
#include "glutil.hh"

//const unsigned InstrumentGraph::max_panels = 10; // Maximum number of arrow lines / guitar frets


InstrumentGraph::InstrumentGraph(Audio& audio, Song const& song, input::DevType inp):
  m_audio(audio), m_song(song), m_input(input::DevType(inp)),
  m_stream(),
  m_cx(0.0, 0.2), m_width(0.5, 0.4),
  m_menu(),
  m_button(getThemePath("button.svg")),
  m_text(getThemePath("sing_timetxt.svg"), config["graphic/text_lod"].f()),
  m_selectedTrack(""),
  m_selectedDifficulty(0),
  m_rejoin(false),
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
	for (size_t i = 0; i < max_panels; ++i) m_pressed[i] = false;
}


void InstrumentGraph::setupPauseMenu() {
	m_menu.clear();
	m_menu.add(MenuOption(_("Resume"), _("Back to performing!")));
	m_menu.add(MenuOption(_("Rejoin"), _("Change selections"), &m_rejoin));
	m_menu.add(MenuOption(_("Restart"), _("Start the song\nfrom the beginning"), "Sing"));
	m_menu.add(MenuOption(_("Quit"), _("Exit to song browser"), "Songs"));
}


void InstrumentGraph::doUpdates() {
	if (!menuOpen() && !m_ready) {
		m_ready = true;
		setupPauseMenu();
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
	else dimensions.screenBottom().middle(m_cx.get()).fixedWidth(std::min(m_width.get(), 0.5));
	// Some helper vars
	ThemeInstrumentMenu& th = *m_menuTheme;
	MenuOptions::const_iterator cur = static_cast<MenuOptions::const_iterator>(&m_menu.current());
	glutil::PushMatrix pm; // Save scaling state
	double w = m_menu.dimensions.w();
	const float s = std::min(m_width.get(), 0.5) / w;
	glScalef(s, s, 1.0f);
	// We need to multiply offset by inverse scale factor to keep it always constant
	// All these vars are ultimately affected by the scaling matrix
	const double offsetX = 0.5f * (dimensions.x1() + dimensions.x2()) / s;
	const float txth = th.option.h();
	const float button_margin = (getGraphType() == input::DANCEPAD ? 0.0f : 0.05f);
	const float step = txth * 0.7f;
	const float h = m_menu.getOptions().size() * step + step;
	float y = -h * .5f + step;
	float x = offsetX - w*.5f + step + button_margin;
	float xx = offsetX + w*.5f - step - button_margin;
	// Background
	th.bg.dimensions.middle(offsetX).center(0).stretch(w, h);
	th.bg.draw();
	// Loop through menu items
	w = 0;
	unsigned i = 0;
	m_button.dimensions.stretch(0.05, 0.05);
	for (MenuOptions::const_iterator it = m_menu.begin(); it != m_menu.end(); ++it, ++i) {
		SvgTxtTheme* txt = &th.option;
		// Selected item?
		if (cur == it) {
			// Draw the key hints
			if (getGraphType() != input::DANCEPAD) {
				if (m_input.isKeyboard()) { // Key letters for keyboard
					txt->dimensions.middle(x - button_margin - m_button.dimensions.w()*0.5f).center(y);
					txt->draw(getGraphType() == input::GUITAR ? "1" : "U");
					txt->dimensions.middle(xx + button_margin - m_button.dimensions.w()*0.5f).center(y);
					txt->draw(getGraphType() == input::GUITAR ? "2" : "P");
				} else { // Colored icons for real instruments
					{
						glutil::Color c(color(getGraphType() == input::GUITAR ? 0 : 1));
						m_button.dimensions.middle(x - button_margin).center(y);
						m_button.draw();
					}
					{
						glutil::Color c(color(getGraphType() == input::GUITAR ? 1 : 4));
						m_button.dimensions.middle(xx + button_margin);
						m_button.draw();
					}
				}
			}
			// Drum up hint
			if (getGraphType() == input::DRUMS && i > 0) {
				if (m_input.isKeyboard()) { // Key letters for keyboard
					txt->dimensions.middle(x - button_margin - m_button.dimensions.w()*0.5f).center(y - step);
					txt->draw("I");
				} else { // Colored icons for real instruments
					glutil::Color c(color(2));
					m_button.dimensions.middle(x - button_margin).center(y - step);
					m_button.draw();
				}
			}
			// Drum down hint
			if (getGraphType() == input::DRUMS && i < m_menu.getOptions().size()-1) {
				if (m_input.isKeyboard()) {
					txt->dimensions.middle(x - button_margin - m_button.dimensions.w()*0.5f).center(y + step);
					txt->draw("O");
				} else {
					glutil::Color c(color(3));
					m_button.dimensions.middle(x - button_margin).center(y + step);
					m_button.draw();
				}
			}
			//th.back_h.dimensions.middle(0.05 + offsetX).center(y);
			//th.back_h.draw();
			// Switch to the "selected" font
			txt = &th.option_selected;
		}
		txt->dimensions.middle(x).center(y);
		txt->draw(it->getName(), it->isActive() ? 1.0f : 0.5f);
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
	// Reset button size
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
		  Color(0.0f, 0.0f, 1.0f), 2.0, m_popupText.get()));
		  --m_countdown;
	}
}


Color const& InstrumentGraph::color(int fret) const {
	static Color fretColors[5] = {
		Color(0.0f, 0.9f, 0.0f),
		Color(0.9f, 0.0f, 0.0f),
		Color(0.9f, 0.9f, 0.0f),
		Color(0.0f, 0.0f, 1.0f),
		Color(0.9f, 0.4f, 0.0f)
	};
	if (fret < 0 || fret >= m_pads) throw std::logic_error("Invalid fret number in InstrumentGraph::color");
	if (getGraphType() == input::DRUMS) {
		if (fret == 0) fret = 4;
		else if (fret == 4) fret = 0;
	}
	return fretColors[fret];
}


void InstrumentGraph::unjoin() {
	m_jointime = getNaN();
	m_rejoin = false;
	m_score = 0;
	m_starmeter = 0;
	m_streak = 0;
	m_longestStreak = 0;
	m_bigStreak = 0;
	m_countdown = 3;
	m_ready = false;
}
