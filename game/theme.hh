#ifndef __THEME_H__
#define __THEME_H__

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include "surface.hh"
#include "opengl_text.hh"
#include <string>

/// theme for song selection
struct ThemeSongs: boost::noncopyable {
	ThemeSongs();
	/// background
	boost::scoped_ptr<Surface> bg;
	/// song display
	boost::scoped_ptr<SvgTxtTheme> song;
	/// ordering display
	boost::scoped_ptr<SvgTxtTheme> order;
};

/// theme for practice screen
struct ThemePractice: boost::noncopyable {
	ThemePractice();
	/// background (score)
	boost::scoped_ptr<Surface> bg;
	/// note
	boost::scoped_ptr<Surface> note;
	/// sharp sign
	boost::scoped_ptr<Surface> sharp;
	/// note name text
	boost::scoped_ptr<SvgTxtTheme> note_txt;
};

/// theme for singing screen
struct ThemeSing: boost::noncopyable {
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
struct ThemeConfiguration: boost::noncopyable {
	ThemeConfiguration();
	/// background
	boost::scoped_ptr<Surface> bg;
	/// configuration item
	boost::scoped_ptr<SvgTxtTheme> item;
	/// configuration value
	boost::scoped_ptr<SvgTxtTheme> value;
};

#endif
