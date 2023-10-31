#pragma once

#include "graphic/opengl_text.hh"
#include "graphic/texture.hh"
#include <string>

/// abstract theme class
class Theme {
protected:
	Theme(const Theme&) = delete;
	Theme(Theme&&) = default;
	Theme& operator=(const Theme&) = delete;
	Theme& operator=(Theme&&) = default;
	Theme();
	Theme(fs::path const& path); ///< creates theme from path
public:
	/// background image for theme
	Texture bg;
};

/// theme for song selection
class ThemeSongs: public Theme {
public:
	ThemeSongs();
	/// song display
	SvgTxtTheme song;
	/// ordering display
	SvgTxtTheme order;
	/// has hiscore display
	SvgTxtTheme has_hiscore;
	/// hiscores display
	SvgTxtTheme hiscores;
};

/// theme for practice screen
class ThemePractice: public Theme {
public:
	ThemePractice();
	/// note
	Texture note;
	/// sharp sign
	Texture sharp;
	/// note name text
	SvgTxtTheme note_txt;
};

/// theme for singing screen
class ThemeSing: public Theme {
public:
	ThemeSing();
	/// top background
	Texture bg_top;
	/// bottom background
	Texture bg_bottom;
	/// current lyrics line
	SvgTxtTheme lyrics_now;
	/// next lyrics line
	SvgTxtTheme lyrics_next;
	/// time display
	SvgTxtTheme timer;
	/// show the current song info
	SvgTxtTheme songinfo;
};

/// theme for audio device screen
class ThemeAudioDevices: public Theme {
public:
	ThemeAudioDevices();
	/// device item
	SvgTxtTheme device;
	/// device item background
	Texture device_bg;
	/// comment text
	SvgTxtTheme comment;
	/// comment background
	Texture comment_bg;
	/// back highlight for selected option
	Texture back_h;
};

/// theme for intro screen
class ThemeIntro: public Theme {
public:
	ThemeIntro();
	/// back highlight for selected option
	Texture back_h;
	/// menu option texts
	std::map<std::string, std::unique_ptr<SvgTxtTheme>> options;
	/// selected menu option text
	SvgTxtTheme option_selected;
	/// menu comment text
	SvgTxtTheme comment;
	/// configuration comment text (short tip)
	SvgTxtTheme short_comment;
	/// notice to remind people the webserver is active
	SvgTxtTheme WebserverNotice;
	/// configuration comment background
	Texture comment_bg;
	/// configuration comment background (short tip)
	Texture short_comment_bg;
	bool colorcycling = true;
	unsigned colorcycleduration = 20;
};

/// theme for instrument menu
class ThemeInstrumentMenu: public Theme {
public:
	ThemeInstrumentMenu();
	/// back highlight for selected option
	Texture back_h;
	/// menu option texts
	std::map<std::string, std::unique_ptr<SvgTxtTheme>> options;
	/// menu selected option text
	SvgTxtTheme option_selected;
	/// menu comment text
	SvgTxtTheme comment;
	/// menu comment background
	//Texture comment_bg;
	/// get a cached option test
	SvgTxtTheme& getCachedOption(const std::string& text);
};

//at the moment just a copy of ThemeSongs
class ThemePlaylistScreen: public Theme {
public:
	ThemePlaylistScreen();
	/// menu option texts
	std::map<std::string, std::unique_ptr<SvgTxtTheme>> options;
	/// selected menu option text
	SvgTxtTheme option_selected;
	/// menu comment text
	SvgTxtTheme comment;
	/// configuration comment text (short tip)
	SvgTxtTheme short_comment;
	/// configuration comment background
	Texture comment_bg;
	/// configuration comment background (short tip)
	Texture short_comment_bg;
};
