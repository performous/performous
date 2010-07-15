#pragma once

#include "opengl_text.hh"
#include "surface.hh"
#include <boost/noncopyable.hpp>
#include <string>

/// abstract theme class
class Theme: boost::noncopyable {
protected:
	Theme();
	Theme(const std::string& path); ///< creates theme from path
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
};

/// theme for options screen
class ThemeConfiguration: public Theme {
public:
	ThemeConfiguration();
	/// configuration item
	SvgTxtTheme item;
	/// configuration value
	SvgTxtTheme value;
	/// configuration comment text
	SvgTxtTheme comment;
	/// configuration comment text (short tip)
	SvgTxtTheme short_comment;
	/// configuration comment background
	Surface comment_bg;
	/// configuration comment background (short tip)
	Surface short_comment_bg;
};

/// theme for intro screen
class ThemeIntro: public Theme {
public:
	ThemeIntro();
	/// back highlight for selected option
	Surface back_h;
	/// menu option text
	boost::ptr_vector<SvgTxtTheme> option;
	/// menu selected option text
	SvgTxtTheme option_selected;
	/// menu comment text
	SvgTxtTheme comment;
	/// menu comment background
	Surface comment_bg;
};
