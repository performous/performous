#include "instrumentgraph.hh"


void InstrumentGraph::drawPopups(double offsetX) {
	for (Popups::iterator it = m_popups.begin(); it != m_popups.end(); ) {
		if (!it->draw(offsetX)) { it = m_popups.erase(it); continue; }
		++it;
	}
}
