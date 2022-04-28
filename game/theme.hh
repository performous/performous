#pragma once

#include "opengl_text.hh"
#include "texture.hh"
#include <memory>
#include <string>

/// abstract theme class
class Theme {
protected:
	Theme(const Theme&) = delete;
  	const Theme& operator=(const Theme&) = delete;
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
	std::shared_ptr<SvgTxtTheme> song;
	/// ordering display
	std::shared_ptr<SvgTxtTheme> order;
	/// has hiscore display
	std::shared_ptr<SvgTxtTheme> has_hiscore;
	/// hiscores display
	std::shared_ptr<SvgTxtTheme> hiscores;
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
	std::shared_ptr<SvgTxtTheme> note_txt;
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
	std::shared_ptr<SvgTxtTheme> lyrics_now;
	/// next lyrics line
	std::shared_ptr<SvgTxtTheme> lyrics_next;
	/// time display
	std::shared_ptr<SvgTxtTheme> timer;
	/// show the current song info
	std::shared_ptr<SvgTxtTheme> songinfo;
};

/// theme for audio device screen
class ThemeAudioDevices: public Theme {
public:
	ThemeAudioDevices();
	/// device item
	std::shared_ptr<SvgTxtTheme> device;
	/// device item background
	Texture device_bg;
	/// comment text
	std::shared_ptr<SvgTxtTheme> comment;
	/// comment background
	Texture comment_bg;
	/// back highlight for selected option
	Texture back_h;
};

/// theme for intro screen
class ThemeIntro: public Theme {
public:
	ThemeIntro(unsigned short int showOpts);
	/// back highlight for selected option
	Texture back_h;
	/// menu option texts
	std::map<std::string, std::shared_ptr<SvgTxtTheme>> options;
	/// selected menu option text
	std::shared_ptr<SvgTxtTheme> option_selected;
	/// menu comment text
	std::shared_ptr<SvgTxtTheme> comment;
	/// configuration comment text (short tip)
	std::shared_ptr<SvgTxtTheme> short_comment;
	/// notice to remind people the webserver is active
	std::shared_ptr<SvgTxtTheme> WebserverNotice;
	/// configuration comment background
	Texture comment_bg;
	/// configuration comment background (short tip)
	Texture short_comment_bg;
};

/// theme for instrument menu
class ThemeInstrumentMenu: public Theme {
public:
	ThemeInstrumentMenu();
	/// back highlight for selected option
	Texture back_h;
	/// menu option texts
	std::map<std::string, std::shared_ptr<SvgTxtTheme>> options;
	/// menu selected option text
	std::shared_ptr<SvgTxtTheme> option_selected;
	/// menu comment text
	std::shared_ptr<SvgTxtTheme> comment;
	/// menu comment background
	//Texture comment_bg;
	/// get a cached option test
	std::shared_ptr<SvgTxtTheme> getCachedOption(const std::string& text);
};

//at the moment just a copy of ThemeSongs
class ThemePlaylistScreen: public Theme {
public:
	ThemePlaylistScreen(unsigned short int showOpts);
	/// menu option texts
	std::map<std::string, std::shared_ptr<SvgTxtTheme>> options;
	/// selected menu option text
	std::shared_ptr<SvgTxtTheme> option_selected;
	/// menu comment text
	std::shared_ptr<SvgTxtTheme> comment;
	/// configuration comment text (short tip)
	std::shared_ptr<SvgTxtTheme> short_comment;
	/// configuration comment background
	Texture comment_bg;
	/// configuration comment background (short tip)
	Texture short_comment_bg;
};
