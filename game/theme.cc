#include "theme.hh"

#include "fs.hh"
#include "configuration.hh"
#include "screen.hh"
#include <cairo.h>
#include <pango/pangocairo.h>

ThemeSongs::ThemeSongs() {
	bg.reset(new Surface(getThemePath("songs_bg.svg")));
	song.reset(new SvgTxtTheme(getThemePath("songs_song.svg"), config["graphic/text_lod"].f()));
	order.reset(new SvgTxtTheme(getThemePath("songs_order.svg"), config["graphic/text_lod"].f()));
}

ThemePractice::ThemePractice() {
	bg.reset(new Surface(getThemePath("practice_bg.svg")));
	note_txt.reset(new SvgTxtTheme(getThemePath("practice_txt.svg"), config["graphic/text_lod"].f()));
	note.reset(new Surface(getThemePath("practice_note.svg")));
	sharp.reset(new Surface(getThemePath("practice_sharp.svg")));
	note->dimensions.fixedHeight(0.03);
	sharp->dimensions.fixedHeight(0.09);
}

ThemeSing::ThemeSing() {
	bg_top.reset(new Surface(getThemePath("sing_bg_top.svg")));
	bg_bottom.reset(new Surface(getThemePath("sing_bg_bottom.svg")));
	lyrics_now.reset(new SvgTxtTheme(getThemePath("sing_lyricscurrent.svg"), config["graphic/text_lod"].f()));
	lyrics_now->setHighlight(getThemePath("sing_lyricshighlight.svg"));
	lyrics_next.reset(new SvgTxtTheme(getThemePath("sing_lyricsnext.svg"), config["graphic/text_lod"].f()));
	timer.reset(new SvgTxtTheme(getThemePath("sing_timetxt.svg"), config["graphic/text_lod"].f()));
}

ThemeConfiguration::ThemeConfiguration() {
	bg.reset(new Surface(getThemePath("configuration_bg.svg")));
	item.reset(new SvgTxtTheme(getThemePath("configuration_item.svg"), config["graphic/text_lod"].f()));
	value.reset(new SvgTxtTheme(getThemePath("configuration_value.svg"), config["graphic/text_lod"].f()));
}

