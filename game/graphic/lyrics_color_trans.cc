#include "lyrics_color_trans.hh"

#include "window.hh"

LyricColorTrans::LyricColorTrans(Window& window, Color const& fill, Color const& stroke, Color const& newFill, Color const& newStroke)
 : m_window(window) {
	window.updateLyricHighlight(fill.linear(), stroke.linear(), newFill.linear(), newStroke.linear());
}

LyricColorTrans::~LyricColorTrans() {
	m_window.clearLyricHighlight();  // Restore the no-recolour sentinel, not the lyric colours
}
