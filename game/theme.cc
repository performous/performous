#include "theme.hh"
#include "configuration.hh"
#include "screen.hh"
#include <cairo.h>
#include <pango/pangocairo.h>

/*
	// Figure out theme folder
	if (theme.find('/') == std::string::npos) {
		ConfigItem::StringList sd = config["system/path_themes"].sl();
		for (std::vector<std::string>::const_iterator it = sd.begin(); it != sd.end(); ++it) {
			fs::path p = *it;
			p /= config["game/theme"].s();
			if (fs::is_directory(p)) { theme = p.string(); break; }
		}
    }
	if (*theme.rbegin() == '/') theme.erase(theme.size() - 1); // Remove trailing slash


std::string ScreenManager::getThemePathFile(std::string const& file) const {
	if (m_theme.empty()) throw std::logic_error("ScreenManager::getThemePathFile(): m_theme is empty");
	return m_theme + "/" + file;
}
*/

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

