#pragma once

#include "configuration.hh"
#include "fs.hh"
#include "graphic/opengl_text.hh"
#include "graphic/texture.hh"
#include "animvalue.hh"

/// class for printing dialogues
class Dialog {
  public:
	/// constructor
	Dialog(std::string const& text);
	/// draws dialogue
	void draw(Window&);

  private:
	std::string m_text;
	Texture m_dialog;
	SvgTxtTheme m_svgText;
	AnimValue m_animationVal;
	enum class State { SLIDEIN, IDLE, SLIDEOUT};
	State m_state;
};
