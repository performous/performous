#pragma once

#include "opengl_text.hh"
#include "surface.hh"
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>

/// abstract theme class
class Theme: boost::noncopyable {
  public:
	/// background image for theme
	boost::scoped_ptr<Surface> bg;
};

/// theme for song selection
struct ThemeSongs: Theme {
	ThemeSongs();
	/// song display
	boost::scoped_ptr<SvgTxtTheme> song;
	/// ordering display
	boost::scoped_ptr<SvgTxtTheme> order;
};

/// theme for practice screen
struct ThemePractice: Theme {
	ThemePractice();
	/// note
	boost::scoped_ptr<Surface> note;
	/// sharp sign
	boost::scoped_ptr<Surface> sharp;
	/// note name text
	boost::scoped_ptr<SvgTxtTheme> note_txt;
};

/// theme for singing screen
struct ThemeSing: Theme {
	ThemeSing();
	/// top background
	boost::scoped_ptr<Surface> bg_top;
	/// bottom background
	boost::scoped_ptr<Surface> bg_bottom;
	/// current lyrics line
	boost::scoped_ptr<SvgTxtTheme> lyrics_now;
	/// next lyrics line
	boost::scoped_ptr<SvgTxtTheme> lyrics_next;
	/// time display
	boost::scoped_ptr<SvgTxtTheme> timer;
};

/// theme for options screen
struct ThemeConfiguration: Theme {
	ThemeConfiguration();
	/// configuration item
	boost::scoped_ptr<SvgTxtTheme> item;
	/// configuration value
	boost::scoped_ptr<SvgTxtTheme> value;
};

