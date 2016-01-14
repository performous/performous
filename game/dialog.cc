#include "dialog.hh"

Dialog::Dialog(std::string const& text) :
	m_text(text),
	m_dialog(findFile("warning.svg")),
	m_svgText(findFile("dialog_txt.svg"), config["graphic/text_lod"].f())
	{
		m_dialog.dimensions.screenTop(-0.1f);
	}

void Dialog::draw() {
	double v = 1.0 - 1.0 * 0; //TODO animate dialog
	m_dialog.dimensions.fixedHeight(0.15).right(0.5).screenTop(-0.10 + 0.11 * v);
	m_dialog.draw();
	m_svgText.dimensions.right(0.35).screenTop(0.08 - 0.10 + 0.11 * v);
	m_svgText.draw(m_text);
}
