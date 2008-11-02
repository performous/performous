#include "theme.hh"
#include "screen.hh"
#include <cairo.h>
#include <pango/pangocairo.h>

CThemeSongs::CThemeSongs() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	bg.reset(new Surface(sm->getThemePathFile("songs_bg.svg")));
	song.reset(new SvgTxtTheme(sm->getThemePathFile("songs_song.svg")));
	order.reset(new SvgTxtTheme(sm->getThemePathFile("songs_order.svg")));
}

CThemePractice::CThemePractice() {
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	bg.reset(new Surface(sm->getThemePathFile("practice_bg.svg")));
	note_txt.reset(new SvgTxtTheme(sm->getThemePathFile("practice_txt.svg")));
	note.reset(new Surface(sm->getThemePathFile("practice_note.svg")));
	sharp.reset(new Surface(sm->getThemePathFile("practice_sharp.svg")));
	note->dimensions.fixedHeight(0.03);
	sharp->dimensions.fixedHeight(0.09);
}

CThemeSing::CThemeSing() {
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	bg_top.reset(new Surface(sm->getThemePathFile("sing_bg_top.svg")));
	bg_bottom.reset(new Surface(sm->getThemePathFile("sing_bg_bottom.svg")));
	lyrics_now.reset(new SvgTxtTheme(sm->getThemePathFile("sing_lyricscurrent.svg")));
	lyrics_now->setHighlight(sm->getThemePathFile("sing_lyricshighlight.svg"));
	lyrics_next.reset(new SvgTxtTheme(sm->getThemePathFile("sing_lyricsnext.svg")));
	timer.reset(new SvgTxtTheme(sm->getThemePathFile("sing_timetxt.svg")));
}

CThemeConfiguration::CThemeConfiguration() {
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	bg.reset(new Surface(sm->getThemePathFile("configuration_bg.svg")));
	item.reset(new SvgTxtTheme(sm->getThemePathFile("configuration_item.svg")));
	value.reset(new SvgTxtTheme(sm->getThemePathFile("configuration_value.svg")));
}

