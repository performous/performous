#pragma once

#include "opengl_text.hh"
#include "surface.hh"
#include "cachemap.hh"
#include <boost/noncopyable.hpp>
#include <string>

/// abstract theme class
class Theme: boost::noncopyable {
protected:
	Theme();
	Theme(fs::path const& path); ///< creates theme from path
public:
	/// background image for theme
	Surface bg;
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
	Surface note;
	/// sharp sign
	Surface sharp;
	/// note name text
	SvgTxtTheme note_txt;
};

/// theme for singing screen
class ThemeSing: public Theme {
public:
	ThemeSing();
	/// top background
	Surface bg_top;
	/// bottom background
	Surface bg_bottom;
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
	Surface device_bg;
	/// comment text
	SvgTxtTheme comment;
	/// comment background
	Surface comment_bg;
	/// back highlight for selected option
	Surface back_h;
};

/// theme for intro screen
class ThemeIntro: public Theme {
public:
	ThemeIntro();
	/// back highlight for selected option
	Surface back_h;
	/// menu option texts
	Cachemap<std::string, SvgTxtTheme> options;
	/// selected menu option text
	SvgTxtTheme option_selected;
	/// menu comment text
	SvgTxtTheme comment;
	/// configuration comment text (short tip)
	SvgTxtTheme short_comment;
	/// notice to remind people the webserver is active
	SvgTxtTheme WebserverNotice;
	/// configuration comment background
	Surface comment_bg;
	/// configuration comment background (short tip)
	Surface short_comment_bg;
};

/// theme for instrument menu
class ThemeInstrumentMenu: public Theme {
public:
	ThemeInstrumentMenu();
	/// back highlight for selected option
	Surface back_h;
	/// menu option texts
	Cachemap<std::string, SvgTxtTheme> options;
	/// menu selected option text
	SvgTxtTheme option_selected;
	/// menu comment text
	SvgTxtTheme comment;
	/// menu comment background
	//Surface comment_bg;
	/// get a cached option test
	SvgTxtTheme& getCachedOption(const std::string& text);
};

//at the moment just a copy of ThemeSongs
class ThemePlaylistScreen: public Theme {
public:
	ThemePlaylistScreen();
	/// menu option texts
	Cachemap<std::string, SvgTxtTheme> options;
	/// selected menu option text
	SvgTxtTheme option_selected;
	/// menu comment text
	SvgTxtTheme comment;
	/// configuration comment text (short tip)
	SvgTxtTheme short_comment;
	/// configuration comment background
	Surface comment_bg;
	/// configuration comment background (short tip)
	Surface short_comment_bg;
};
