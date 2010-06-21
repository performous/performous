#include "instrumentgraph.hh"

//const unsigned InstrumentGraph::max_panels = 10; // Maximum number of arrow lines / guitar frets


void InstrumentGraph::drawMenu(double offsetX) {
	if (m_menu.options.empty()) return;
	float step = 0.05;
	float y = -0.5 * m_menu.options.size() * step;
	for (InstrumentMenuOptions::iterator it = m_menu.options.begin(); it != m_menu.options.end(); ++it) {
		// TODO: Use theme
		m_text.dimensions.center(y).middle(-0.09 + offsetX);
		std::string value = it->value;
		if (m_menu.current == it) value = ">  " + value + "  <";
		m_text.draw(value);
		y += step;
	}
	m_text.dimensions.center(y+3*step).middle(-0.09 + offsetX);
	m_text.draw(m_menu.current->name);
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
