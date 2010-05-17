#include "instrumentgraph.hh"


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
