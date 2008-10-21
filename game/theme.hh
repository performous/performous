#ifndef __THEME_H__
#define __THEME_H__

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include "surface.hh"
#include "opengl_text.hh"
#include <string>

struct CThemeSongs: boost::noncopyable {
	CThemeSongs();
	boost::scoped_ptr<Surface> bg;
	boost::scoped_ptr<SvgTxtTheme> song;
	boost::scoped_ptr<SvgTxtTheme> order;
};

struct CThemePractice: boost::noncopyable {
	CThemePractice();
	boost::scoped_ptr<Surface> bg;
	boost::scoped_ptr<Surface> note;
	boost::scoped_ptr<Surface> sharp;
	boost::scoped_ptr<SvgTxtTheme> note_txt;
};

struct CThemeSing: boost::noncopyable {
	CThemeSing();
	boost::scoped_ptr<Surface> bg;
	boost::scoped_ptr<SvgTxtTheme> lyrics_now;
	boost::scoped_ptr<SvgTxtTheme> lyrics_next;
	boost::scoped_ptr<SvgTxtTheme> score1;
	boost::scoped_ptr<SvgTxtTheme> score2;
	boost::scoped_ptr<SvgTxtTheme> timer;
};

struct CThemeConfiguration: boost::noncopyable {
	CThemeConfiguration();
	boost::scoped_ptr<Surface> bg;
	boost::scoped_ptr<SvgTxtTheme> item;
	boost::scoped_ptr<SvgTxtTheme> value;
};

#endif
