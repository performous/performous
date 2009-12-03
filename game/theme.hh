#pragma once

#include "opengl_text.hh"
#include "surface.hh"
#include <boost/noncopyable.hpp>
#include <string>

/// abstract theme class
class Theme: boost::noncopyable {
  protected:
  	Theme();
  	Theme(const std::string path); ///< creates theme from path
  public:
	/// background image for theme
	Surface bg;
};

/// theme for song selection
struct ThemeSongs: Theme {
	ThemeSongs();
	/// song display
	SvgTxtTheme song;
	/// ordering display
	SvgTxtTheme order;
};

/// theme for practice screen
struct ThemePractice: Theme {
	ThemePractice();
	/// note
	Surface note;
	/// sharp sign
	Surface sharp;
	/// note name text
	SvgTxtTheme note_txt;
};

/// theme for singing screen
struct ThemeSing: Theme {
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
struct ThemeConfiguration: Theme {
	ThemeConfiguration();
	/// configuration item
	SvgTxtTheme item;
	/// configuration value
	SvgTxtTheme value;
};

/// theme for intro screen
struct ThemeIntro: Theme {
	ThemeIntro();
	/// back highlight
	Surface back_h;
	/// menu option
	SvgTxtTheme option;
	/// menu comment
	SvgTxtTheme comment;
};
