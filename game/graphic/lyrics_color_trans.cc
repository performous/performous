#include "lyrics_color_trans.hh"

#include "window.hh"

LyricColorTrans::LyricColorTrans(Window& window, Color const& fill, Color const& stroke, Color const& newFill, Color const& newStroke)
 : m_window(window) {
	oldFill = fill.linear();
	oldStroke = stroke.linear();
	window.updateLyricHighlight(fill.linear(), stroke.linear(), newFill.linear(), newStroke.linear());
}

LyricColorTrans::~LyricColorTrans() {
	m_window.updateLyricHighlight(oldFill, oldStroke);
}
