#ifndef __OPENGL_THEME_H__
#define __OPENGL_THEME_H__

#include "theme.hh"
#include "surface.hh"
#include <boost/scoped_ptr.hpp>

// Special theme for creating opengl themed surfaces
// this structure does not include:
//  - library specific field
//  - global positionning
// the font{family,style,weight,align} are the one found into SVGs
struct TThemeTxtOpenGL {
	TRGBA fill_col;
	TRGBA stroke_col;
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
	OpenGLText(TThemeTxtOpenGL _text);
	~OpenGLText();
	void draw(Dimensions &_dim, TexCoords &_tex);
	private:
	unsigned int x_power_of_two;
	unsigned int y_power_of_two;
	double x;
	double y;
	double x_advance;
	double y_advance;
	// TODO: only have a texture id uploaded here
	// because Surface do way too much things (global positionning for example)
	boost::scoped_ptr<OpenGLTexture> m_texture;
};

#endif // __OPENGL_TEXT_H__
