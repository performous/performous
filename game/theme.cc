#include "theme.hh"
#include "configuration.hh"
#include "screen.hh"
#include <cairo.h>
#include <pango/pangocairo.h>

ThemeSongs::ThemeSongs() {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	bg.reset(new Surface(sm->getThemePathFile("songs_bg.svg")));
	song.reset(new SvgTxtTheme(sm->getThemePathFile("songs_song.svg"), config["graphic/text_lod"].f()));
	order.reset(new SvgTxtTheme(sm->getThemePathFile("songs_order.svg"), config["graphic/text_lod"].f()));
}

ThemePractice::ThemePractice() {
	ScreenManager * sm = ScreenManager::getSingletonPtr();
	bg.reset(new Surface(sm->getThemePathFile("practice_bg.svg")));
	note_txt.reset(new SvgTxtTheme(sm->getThemePathFile("practice_txt.svg"), config["graphic/text_lod"].f()));
	note.reset(new Surface(sm->getThemePathFile("practice_note.svg")));
	sharp.reset(new Surface(sm->getThemePathFile("practice_sharp.svg")));
	note->dimensions.fixedHeight(0.03);
	sharp->dimensions.fixedHeight(0.09);
}

ThemeSing::ThemeSing() {
	ScreenManager * sm = ScreenManager::getSingletonPtr();
	bg_top.reset(new Surface(sm->getThemePathFile("sing_bg_top.svg")));
	bg_bottom.reset(new Surface(sm->getThemePathFile("sing_bg_bottom.svg")));
	lyrics_now.reset(new SvgTxtTheme(sm->getThemePathFile("sing_lyricscurrent.svg"), config["graphic/text_lod"].f()));
	lyrics_now->setHighlight(sm->getThemePathFile("sing_lyricshighlight.svg"));
	lyrics_next.reset(new SvgTxtTheme(sm->getThemePathFile("sing_lyricsnext.svg"), config["graphic/text_lod"].f()));
	timer.reset(new SvgTxtTheme(sm->getThemePathFile("sing_timetxt.svg"), config["graphic/text_lod"].f()));
}

ThemeConfiguration::ThemeConfiguration() {
	ScreenManager * sm = ScreenManager::getSingletonPtr();
	bg.reset(new Surface(sm->getThemePathFile("configuration_bg.svg")));
	item.reset(new SvgTxtTheme(sm->getThemePathFile("configuration_item.svg"), config["graphic/text_lod"].f()));
	value.reset(new SvgTxtTheme(sm->getThemePathFile("configuration_value.svg"), config["graphic/text_lod"].f()));
}

