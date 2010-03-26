#include "instrumentgraph.hh"


void InstrumentGraph::drawPopups(double time, double offsetX, Dimensions dimensions) {
	// Draw streak pop-up for long streak intervals
	double streakAnim = m_streakPopup.get();
	if (streakAnim > 0.0) {
		double s = 0.2 * (1.0 + streakAnim);
		glColor4f(1.0f, 0.0f, 0.0f, 1.0 - streakAnim);
		m_popupText->render(boost::lexical_cast<std::string>(unsigned(m_bigStreak)) + "\nStreak!");
		m_popupText->dimensions().center(0.1).middle(offsetX).stretch(s,s);
		m_popupText->draw();
		if (streakAnim > 0.999) m_streakPopup.setTarget(0.0, true);
	}
	// Draw godmode activation pop-up
	double godAnim = m_godmodePopup.get();
	if (godAnim > 0.0) {
		float a = 1.0 - godAnim;
		float s = 0.2 * (1.0 + godAnim);
		glColor4f(0.3f, 0.0f, 1.0f, a);
		m_popupText->render("God Mode\nActivated!");
		m_popupText->dimensions().center(0.1).middle(offsetX).stretch(s,s);
		m_popupText->draw();
		m_text.dimensions.screenBottom(-0.02).middle(-0.12 + offsetX);
		m_text.draw("Mistakes ignored!", 1.0 - godAnim);
		if (godAnim > 0.999) m_godmodePopup.setTarget(0.0, true);
	}
}
