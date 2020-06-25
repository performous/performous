#include "dialog.hh"

Dialog::Dialog(std::string const& text) :
	m_text(text),
	m_dialog(findFile("warning.svg")),
	m_svgText(findFile("dialog_txt.svg"), config["graphic/text_lod"].f(), WrappingStyle().menuScreenText())
	{
		m_dialog.dimensions.screenTop(-0.1f);
		m_animationVal.setValue(1);
		m_state = SLIDEIN;
	}

void Dialog::draw() {
	double verticaloffset = 1.0;
	switch(m_state) {
	case IDLE:
		verticaloffset = 0.0;
		if(m_animationVal.get() == 0) {
			m_state = SLIDEOUT;
			m_animationVal.setValue(1);
		}
		break;
		case SLIDEIN :
		verticaloffset = 1.0 - 1.0 * (1.0-m_animationVal.get()); //TODO animate dialog
		if(m_animationVal.get() == 0) {
			m_state = IDLE;
			m_animationVal.setValue(6);
		}
		break;
		case SLIDEOUT:
			verticaloffset = 1.0 - 1.0 * m_animationVal.get();
		break;
	}


	m_dialog.dimensions.fixedHeight(0.15).right(0.5).screenTop(-0.10 + 0.11 - verticaloffset);
	m_dialog.draw();
	m_svgText.dimensions.right(0.35).screenTop(0.08 - 0.10 + 0.11 - verticaloffset);
	m_svgText.draw(m_text);
}
