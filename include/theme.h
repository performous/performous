#ifndef __THEME_H__
#define __THEME_H__

#include "../config.h"

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <cairosvg.h>
#include <surface.h>
#include <string>

struct TRGBA {
	double r;
	double g;
	double b;
	double a;
	TRGBA(): r(), g(), b(), a() {}
};

struct TThemeTxt {
	double x;
	double y;
	double svg_width;
	double svg_height;
	cairo_text_extents_t extents;
	TRGBA fill_col;
	TRGBA stroke_col;
	double stroke_width;
	double fontsize;
	std::string fontfamily;
	cairo_font_slant_t fontstyle;
	cairo_font_weight_t fontweight;
	double scale;
	std::string text;
	TThemeTxt(): x(), y(), svg_width(), svg_height(), stroke_width(), fontsize(), scale() {}
};

struct TThemeRect {
	double x;
	double y;
	double svg_width;
	double svg_height;
	double final_width;
	double final_height;
	double width;
	double height;
	TRGBA fill_col;
	TRGBA stroke_col;
	double stroke_width;
	TThemeRect(): x(), y(), svg_width(), svg_height(), final_width(), final_height(), width(), height(), stroke_width() {}
};

class CTheme: boost::noncopyable {
  public:
	CTheme(int width, int height);
	~CTheme();
	cairo_surface_t *PrintText(TThemeTxt *text); 
	cairo_surface_t *DrawRect(TThemeRect rect);
	cairo_text_extents_t GetTextExtents(TThemeTxt text);
	cairo_surface_t* getCurrent() {return this->surface;}
	void ParseSVGForText(std::string const& filename, TThemeTxt *text);
	void ParseSVGForRect(std::string const& filename, TThemeRect *rect);
	void clear();
  private: 
	int width;
	int height;
	cairo_surface_t *surface;
	cairo_t *dc;
	void walk_tree(xmlNode * a_node, TThemeTxt *text);
	void walk_tree(xmlNode * a_node, TThemeRect *rect);
	void getcolor(char *string, TRGBA *col);
};

struct CThemeSongs: boost::noncopyable {
	CThemeSongs(unsigned int width, unsigned int height);
	~CThemeSongs();
	boost::scoped_ptr<Surface> bg;
	boost::scoped_ptr<CTheme> theme;
	TThemeTxt song;
	TThemeTxt order;
};

struct CThemePractice: boost::noncopyable {
	CThemePractice(unsigned int width, unsigned int height);
	~CThemePractice();
	boost::scoped_ptr<Surface> bg;
	boost::scoped_ptr<CTheme> theme;
	TThemeTxt notetxt;
	TThemeRect peak;
};

struct CThemeSing: boost::noncopyable {
	CThemeSing(unsigned int width, unsigned int height);
	~CThemeSing();
	boost::scoped_ptr<Surface> bg;
	boost::scoped_ptr<CairoSVG> p1box;
	boost::scoped_ptr<CTheme> theme;
	TThemeTxt timertxt; 
	TThemeTxt p1score;
	TThemeTxt lyricspast;
	TThemeTxt lyricsfuture;  
	TThemeTxt lyricshighlight;
	TThemeTxt lyricsnextsentence;
	TThemeRect progressfg;
	TThemeRect tostartfg;
};

struct CThemeScore: boost::noncopyable {
	CThemeScore(unsigned int width, unsigned int height);
	~CThemeScore();
	boost::scoped_ptr<CairoSVG> bg;
	boost::scoped_ptr<CTheme> theme;
	TThemeTxt normal_score;
	TThemeTxt rank;
	TThemeRect level;
};

struct CThemeConfiguration: boost::noncopyable {
	CThemeConfiguration(unsigned int width, unsigned int height);
	~CThemeConfiguration();
	boost::scoped_ptr<Surface> bg;
	boost::scoped_ptr<CTheme> theme;
	TThemeTxt item;
	TThemeTxt value;
};

#endif
