#pragma once

#include "opengl_text.hh"
#include "fs.hh"
#include "surface.hh"
#include "configuration.hh"

/// class for printing dialogues
class Dialog {
  public:
	/// constructor
	Dialog(std::string const& text):
	  m_text(text),
	  m_dialog(getThemePath("warning.svg")),
	  m_svgText(getThemePath("dialog_txt.svg"), config["graphic/text_lod"].f())
	{
		m_dialog.dimensions.fixedWidth(0.8);
	}
	/// draws dialogue
	void draw() {
		glutil::PushMatrix pm;
		glTranslatef(0.0f, 0.0f, 0.1f);  // Raise a bit in 3D
		m_dialog.draw();
		m_svgText.draw(m_text);
	}

  private:
	std::string m_text;
	Surface m_dialog;
	SvgTxtTheme m_svgText;
};
