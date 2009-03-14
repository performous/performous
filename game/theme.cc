#include "theme.hh"
#include "configuration.hh"
#include "screen.hh"
#include <cairo.h>
#include <pango/pangocairo.h>

ThemeSongs::ThemeSongs() {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	bg.reset(new Surface(sm->getThemePathFile("songs_bg.svg"), config["graphic/svg_lod"].get_f()));
	song.reset(new SvgTxtTheme(sm->getThemePathFile("songs_song.svg"), config["graphic/text_lod"].get_f()));
	order.reset(new SvgTxtTheme(sm->getThemePathFile("songs_order.svg"), config["graphic/text_lod"].get_f()));
}

ThemePractice::ThemePractice() {
	ScreenManager * sm = ScreenManager::getSingletonPtr();
	bg.reset(new Surface(sm->getThemePathFile("practice_bg.svg"), config["graphic/svg_lod"].get_f()));
	note_txt.reset(new SvgTxtTheme(sm->getThemePathFile("practice_txt.svg"), config["graphic/text_lod"].get_f()));
	note.reset(new Surface(sm->getThemePathFile("practice_note.svg"), config["graphic/svg_lod"].get_f()));
	sharp.reset(new Surface(sm->getThemePathFile("practice_sharp.svg"), config["graphic/svg_lod"].get_f()));
	note->dimensions.fixedHeight(0.03);
	sharp->dimensions.fixedHeight(0.09);
}

ThemeSing::ThemeSing() {
	ScreenManager * sm = ScreenManager::getSingletonPtr();
	bg_top.reset(new Surface(sm->getThemePathFile("sing_bg_top.svg"), config["graphic/svg_lod"].get_f()));
	bg_bottom.reset(new Surface(sm->getThemePathFile("sing_bg_bottom.svg"), config["graphic/svg_lod"].get_f()));
	lyrics_now.reset(new SvgTxtTheme(sm->getThemePathFile("sing_lyricscurrent.svg"), config["graphic/text_lod"].get_f()));
	lyrics_now->setHighlight(sm->getThemePathFile("sing_lyricshighlight.svg"));
	lyrics_next.reset(new SvgTxtTheme(sm->getThemePathFile("sing_lyricsnext.svg"), config["graphic/text_lod"].get_f()));
	timer.reset(new SvgTxtTheme(sm->getThemePathFile("sing_timetxt.svg"), config["graphic/text_lod"].get_f()));
}

ThemeConfiguration::ThemeConfiguration() {
	ScreenManager * sm = ScreenManager::getSingletonPtr();
	bg.reset(new Surface(sm->getThemePathFile("configuration_bg.svg"), config["graphic/svg_lod"].get_f()));
	item.reset(new SvgTxtTheme(sm->getThemePathFile("configuration_item.svg"), config["graphic/text_lod"].get_f()));
	value.reset(new SvgTxtTheme(sm->getThemePathFile("configuration_value.svg"), config["graphic/text_lod"].get_f()));
}

