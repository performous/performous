#pragma once

#include "color.hh"
#include "surface.hh"
#include <boost/scoped_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <vector>

/// Load custom fonts from current theme and data folders
void loadFonts();

/// zoomed text
struct TZoomText {
	/// text
	std::string string;
	/// zoomfactor
	double factor;
	/// start of word?
	bool wordStart;
	/// constructor
	TZoomText(std::string const& str = std::string()): string(str), factor(1.0), wordStart(false) {}
	void updateWordStart (bool param = false) { wordStart = param; };
	void addSpace () { string = " " + string; };
};

/// Special theme for creating opengl themed surfaces
/** this structure does not include:
 *  - library specific field
 *  - global positionning
 * the font{family,style,weight,align} are the one found into SVGs
 */
struct TextStyle {
	Color fill_col; ///< fill color
	Color stroke_col; ///< stroke color
	double stroke_width; ///< stroke thickness
	double fontsize; ///< fontsize
	std::string fontfamily; ///< fontfamily
	std::string fontstyle; ///< fontstyle
	std::string fontweight; ///< fontweight
	std::string fontalign; ///< alignment
	std::string text; ///< text
	TextStyle(): stroke_width(), fontsize() {}
};

/// this class will enable to create a texture from a themed text structure
/** it will not cache any data (class using this class should)
 * it provides size of the texture are drawn (x,y)
 * it provides size of the texture created (x_power_of_two, y_power_of_two)
 */
class OpenGLText {
public:
	/// constructor
	OpenGLText(TextStyle &_text, double m);
	/// draws area
	void draw(Dimensions &_dim, TexCoords &_tex);
	/// draws full texture
	void draw();
	/// @return x
	double x() const { return m_x; }
	/// @return y
	double y() const { return m_y; }
	/// @returns dimension of texture
	Dimensions& dimensions() { return m_surface.dimensions; }

private:
	double m_x;
	double m_y;
	double m_x_advance;
	double m_y_advance;
	Surface m_surface;
};

/// themed svg texts (simple)
class SvgTxtThemeSimple {
public:
	/// constructor
	SvgTxtThemeSimple(fs::path const& themeFile, double factor = 1.0);
	/// renders text
	void render(std::string _text);
	/// draws texture
	void draw();
	/// gets dimensions
	Dimensions& dimensions() { return m_opengl_text->dimensions(); }

private:
	boost::scoped_ptr<OpenGLText> m_opengl_text;
	std::string m_cache_text;
	TextStyle m_text;
	double m_factor;
};

/// themed svg texts
class SvgTxtTheme {
public:
	/// enum declaration Gravity:
	/** <pre>
	 * +----+---+----+
	 * | NW | N | NE |
	 * +----+---+----+
	 * | W  | C |  E |
	 * +----+---+----+
	 * | SW | S | SE |
	 * +----+---+----+ (and ASIS)
	 *
	 * Coord:
	 * (x0,y0)        (x1,y0)
	 *    +--------------+
	 *    |              |
	 *    |              |
	 *    +--------------+
	 * (x0,y1)        (x1,y1)
	 *
	 * gravity will affect how fit-inside/fit-outside will work
	 * fitting will always keep aspect ratio
	 * fit-inside: will fit inside if texture is too big
	 * force-fit-inside: will always stretch to fill at least one of the axis
	 * fit-outside: will fit outside if texture is too small
	 * force-fit-outside: will always stretch to fill both axis
	 * gravity does not mean position, it is only an anchor
	 * Fixed points:
	 *   NW: (x0,y0)
	 *   N:  ((x0+x1)/2,y0)
	 *   NE: (x1,y0)
	 *   W:  (x0,(y0+y1)/2)
	 *   C:  ((x0+x1)/2,(y0+y1)/2)
	 *   E:  (x1,(y0+y1)/2)
	 *   SW: (x0,y1)
	 *   S:  ((x0+x1)/2,y1)
	 *   SE: (x1,y1)</pre>
	 */
	/// TODO anchors
	enum Gravity {NW,N,NE,W,C,E,SW,S,SE};
	/// where to position when space is too small
	enum Fitting {F_ASIS, INSIDE, OUTSIDE, FORCE_INSIDE, FORCE_OUTSIDE};
	/// vertical align
	enum VAlign {V_ASIS, TOP, MIDDLE, BOTTOM};
	/// horizontal align
	enum Align {A_ASIS, LEFT, CENTER, RIGHT};
	/// dimensions, what else
	Dimensions dimensions;
	/// constructor
	SvgTxtTheme(fs::path const& themeFile, double factor = 1.0);
	/// draws text with alpha
	void draw(std::vector<TZoomText>& _text);
	/// draw texts
// 	void draw(std::vector<std::string> const& _text);
	/// draw text with alpha
	void draw(std::string _text);
	/// sets highlight
	void setHighlight(fs::path const& themeFile);
	/// width
	double w() const { return m_texture_width; }
	/// height
	double h() const { return m_texture_height; }
	/// set align
	void setAlign(Align align) { m_align = align; }

private:
	boost::ptr_vector<OpenGLText> m_opengl_text;
	Align m_align;
	double m_x;
	double m_y;
	double m_width;
	double m_height;
	double m_factor;
	double m_texture_width;
	double m_texture_height;
	std::string m_cache_text;
	TextStyle m_text;
	TextStyle m_text_highlight;
};

