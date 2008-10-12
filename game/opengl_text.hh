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
	double x(void) {return m_x;};
	double y(void) {return m_y;};
	double x_advance(void) {return m_x_advance;};
	double y_advance(void) {return m_y_advance;};
  private:
	unsigned int m_x_power_of_two;
	unsigned int m_y_power_of_two;
	double m_x;
	double m_y;
	double m_x_advance;
	double m_y_advance;
	// TODO: only have a texture id uploaded here
	// because Surface do way too much things (global positionning for example)
	boost::scoped_ptr<OpenGLTexture> m_texture;
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
  	SvgTxtTheme(std::string _theme_file, Align _a=A_ASIS, VAlign _v=V_ASIS, Gravity _g=NW, Fitting _f=F_ASIS);
	void draw(std::vector<TZoomText> _text);
	void draw(std::vector<std::string> _text);
	void draw(std::string _text);
	void setGravity(Gravity _g) {m_gravity=_g;};
	void setFitting(Fitting _f) {m_fitting=_f;};
  private:
  	boost::scoped_ptr<OpenGLText> m_opengl_text[50];
	Gravity m_gravity;
	Fitting m_fitting;
	VAlign m_valign;
	Align m_align;
	double m_x;
	double m_y;
	double m_width;
	double m_height;
	std::string m_cache_text;
	TThemeTxtOpenGL m_text;
};


#endif // __OPENGL_TEXT_H__
