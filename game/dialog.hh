#pragma once

#include "configuration.hh"
#include "fs.hh"
#include "opengl_text.hh"
#include "surface.hh"

/// class for printing dialogues
class Dialog {
  public:
	/// constructor
	Dialog(std::string const& text);
	/// draws dialogue
	void draw();

  private:
	std::string m_text;
	Surface m_dialog;
	SvgTxtTheme m_svgText;
};
