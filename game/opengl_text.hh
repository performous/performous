#ifndef __OPENGL_THEME_H__
#define __OPENGL_THEME_H__

#include "color.hh"
#include "surface.hh"
#include <boost/scoped_ptr.hpp>
#include <vector>

struct TZoomText {
	std::string string;
	double factor;
	TZoomText(): string(),factor() {};
};

// Special theme for creating opengl themed surfaces
// this structure does not include:
//  - library specific field
//  - global positionning
// the font{family,style,weight,align} are the one found into SVGs
struct TThemeTxtOpenGL {
	Color fill_col;
	Color stroke_col;
	double stroke_width;
	double fontsize;
	std::string fontfamily;
	std::string fontstyle;
	std::string fontweight;
	std::string fontalign;
	std::string text;
	TThemeTxtOpenGL(): stroke_width(), fontsize() {}
};

// this class will enable to create a texture from a themed text structure
// it will not cache any data (class using this class should)
// it provides size of the texture are drawn (x,y)
// it provides size of the texture created (x_power_of_two, y_power_of_two)
class OpenGLText {
  public:
	OpenGLText(TThemeTxtOpenGL &_text);
	void draw(Dimensions &_dim, TexCoords &_tex);
	void draw();
	double x() {return m_x;};
	double y() {return m_y;};
	double x_advance() {return m_x_advance;};
	double y_advance() {return m_y_advance;};
	Dimensions& dimensions() { return m_surface.dimensions; }
  private:
	double m_x;
	double m_y;
	double m_x_advance;
	double m_y_advance;
	Surface m_surface;
};

class SvgTxtThemeSimple {
  public:
	SvgTxtThemeSimple(std::string _theme_file);
	void render(std::string _text);
	void draw();
	Dimensions& dimensions() { return m_opengl_text->dimensions(); }
  private:
	boost::scoped_ptr<OpenGLText> m_opengl_text;
	std::string m_cache_text;
	TThemeTxtOpenGL m_text;
	double m_x;
	double m_y;
	double m_width;
	double m_height;
};

// Gravity:
//
// +----+---+----+
// | NW | N | NE |
// +----+---+----+
// | W  | C |  E |
// +----+---+----+
// | SW | S | SE |
// +----+---+----+ (and ASIS)
//
// Coord:
// (x0,y0)        (x1,y0)
//    +--------------+
//    |              |
//    |              |
//    +--------------+
// (x0,y1)        (x1,y1)
//
// gravity will affect how fit-inside/fit-outside will work
// fitting will always keep aspect ratio
// fit-inside: will fit inside if texture is too big
// force-fit-inside: will always stretch to fill at least one of the axis
// fit-outside: will fit outside if texture is too small
// force-fit-outside: will always stretch to fill both axis
// gravity does not mean position, it is only an anchor
// Fixed points:
//   NW: (x0,y0)
//   N:  ((x0+x1)/2,y0)
//   NE: (x1,y0)
//   W:  (x0,(y0+y1)/2)
//   C:  ((x0+x1)/2,(y0+y1)/2)
//   E:  (x1,(y0+y1)/2)
//   SW: (x0,y1)
//   S:  ((x0+x1)/2,y1)
//   SE: (x1,y1)
class SvgTxtTheme {
  public:
	// enum declaration
	enum Gravity {NW,N,NE,W,C,E,SW,S,SE};
	enum Fitting {F_ASIS, INSIDE, OUTSIDE, FORCE_INSIDE, FORCE_OUTSIDE};
	enum VAlign {V_ASIS, TOP, MIDDLE, BOTTOM};
	enum Align {A_ASIS, LEFT, CENTER, RIGHT};
	//
	Dimensions dimensions;
  	SvgTxtTheme(std::string _theme_file);
	void draw(std::vector<TZoomText> _text);
	void draw(std::vector<std::string> _text);
	void draw(std::string _text);
	void setHighlight(std::string _theme_file);
  private:
	boost::scoped_ptr<OpenGLText> m_opengl_text[50]; // FIXME: "large enough value"
	Align m_align;
	double m_x;
	double m_y;
	double m_width;
	double m_height;
	std::string m_cache_text;
	TThemeTxtOpenGL m_text;
	TThemeTxtOpenGL m_text_highlight;
};


#endif // __OPENGL_TEXT_H__
