#include "theme.hh"
#include "screen.hh"
#include <cairo.h>
#include <pango/pangocairo.h>

CThemeSongs::CThemeSongs() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	bg.reset(new Surface(sm->getThemePathFile("songs_bg.svg")));
	song.reset(new SvgTxtTheme(sm->getThemePathFile("songs_song.svg"),SvgTxtTheme::CENTER));
	order.reset(new SvgTxtTheme(sm->getThemePathFile("songs_order.svg"),SvgTxtTheme::CENTER));
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
	bg.reset(new Surface(sm->getThemePathFile("sing_bg.svg")));
	lyrics_now.reset(new SvgTxtTheme(sm->getThemePathFile("sing_lyricscurrent.svg"),SvgTxtTheme::CENTER));
	lyrics_now->setHighlight(sm->getThemePathFile("sing_lyricshighlight.svg"));
	lyrics_next.reset(new SvgTxtTheme(sm->getThemePathFile("sing_lyricsnext.svg"),SvgTxtTheme::CENTER));
	timer.reset(new SvgTxtTheme(sm->getThemePathFile("sing_timetxt.svg"),SvgTxtTheme::CENTER,SvgTxtTheme::TOP));
}

CThemeConfiguration::CThemeConfiguration() {
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	bg.reset(new Surface(sm->getThemePathFile("configuration_bg.svg")));
	item.reset(new SvgTxtTheme(sm->getThemePathFile("configuration_item.svg"),SvgTxtTheme::CENTER));
	value.reset(new SvgTxtTheme(sm->getThemePathFile("configuration_value.svg"),SvgTxtTheme::CENTER));
}

