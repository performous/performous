#include "dialog.hh"

Dialog::Dialog(std::string const& text) :
	m_text(text),
	m_dialog(findFile("warning.svg")),
	m_svgText(findFile("dialog_txt.svg"), config["graphic/text_lod"].f())
	{
		m_dialog.dimensions.fixedWidth(0.8);
	}

void Dialog::draw() {
	using namespace glmath;
	Transform(translate(vec3(0.0f, 0.0f, 0.1f)));  // Raise a bit in 3D
	m_dialog.draw();
	m_svgText.draw(m_text);
}
