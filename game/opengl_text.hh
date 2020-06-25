#pragma once

#include "color.hh"
#include "texture.hh"
#include "unicode.hh"
#include <pango/pangocairo.h>
#include <vector>

/// Load custom fonts from current theme and data folders
void loadFonts();

/// zoomed text
struct TZoomText {
	/// text
	std::string string;
	/// zoomfactor
	double factor;
	/// constructor
	TZoomText(std::string const& str = std::string()): string(str), factor(1.0) {}
};

/// Special theme for creating opengl themed surfaces
/** this structure does not include:
 *  - library specific field
 *  - global positionning
 * the font{family,style,weight,align} are the one found into SVGs
 */
struct TextStyle {
	cairo_line_join_t LineJoin() {	///< convert svg string to cairo enum.
		if (UnicodeUtil::toLower(stroke_linejoin) == "round") return CAIRO_LINE_JOIN_ROUND;
		if (UnicodeUtil::toLower(stroke_linejoin) == "bevel") return CAIRO_LINE_JOIN_BEVEL;	
		return CAIRO_LINE_JOIN_MITER;
	};
	cairo_line_cap_t LineCap() {	///< convert svg string to cairo enum.
		if (UnicodeUtil::toLower(stroke_linecap) == "round") return CAIRO_LINE_CAP_ROUND;
		if (UnicodeUtil::toLower(stroke_linecap) == "square") return CAIRO_LINE_CAP_SQUARE;	
		return CAIRO_LINE_CAP_BUTT;
	};
	Color fill_col; ///< fill color
	Color stroke_col; ///< stroke color
	double stroke_width; ///< stroke thickness
	double stroke_miterlimit; ///< stroke miter limit
	double fontsize; ///< fontsize
	std::string fontfamily; ///< fontfamily
	std::string fontstyle; ///< fontstyle
	std::string fontweight; ///< fontweight
	std::string fontalign; ///< alignment
	std::string	stroke_linejoin; ///< stroke line-join type
	std::string	stroke_linecap; ///< stroke line-join type
	std::string text; ///< text
	TextStyle(): stroke_width(), stroke_miterlimit(1.0), fontsize() {}
};
	
/// Convenience container for deciding how a given OpenGLText instance will be wrapped, ellipsized or fitted to the display area.
struct WrappingStyle {
	enum class EllipsizeMode { NONE, START, MIDDLE, END };

	/// constructor
	WrappingStyle(unsigned short int _maxWidth = 0, EllipsizeMode _ellipsize = EllipsizeMode::NONE, unsigned short int _maxLines = 1);
	
	/// setters
	WrappingStyle& ellipsizeNone(unsigned short int lines = 1) { m_maxLines = (lines * -1); m_ellipsize = EllipsizeMode::NONE; return *this; }
	WrappingStyle& ellipsizeStart(unsigned short int lines = 1) { m_maxLines = (lines * -1); m_ellipsize = EllipsizeMode::START; return *this; }
	WrappingStyle& ellipsizeMiddle(unsigned short int lines = 1) { m_maxLines = (lines * -1); m_ellipsize = EllipsizeMode::MIDDLE; return *this; }
	WrappingStyle& ellipsizeEnd(unsigned short int lines = 1) { m_maxLines = (lines * -1); m_ellipsize = EllipsizeMode::END; return *this; }
	WrappingStyle& setWidth(unsigned short int width = 0) { m_maxWidth = (width > 96 ? 0 : width); return *this; }
	
	/// presets
	WrappingStyle& menuScreenText(unsigned short int lines = 0) { setWidth(96); ellipsizeMiddle(lines); return *this;  } ///< No line limit, wrap at screen edge and/or ellipsize middle.
	WrappingStyle& lyrics() { ellipsizeNone(1); setWidth(96); return *this;  } ///< Default one line, wrap at screen edge, don't ellipsize or wrap.
	
	/** maximum lines.
	  * This is an int because Pango is a mess.
	  * Values (for Pango):
	  *   <=-1: Maximum amount of lines per paragraph.
	  *      0: Maximum one line in the entire layout.
	  *    >=1: Maximum height in PANGO_UNITS.
	  * We'll always be using negative values, and we'll use the "0" to signal we should not set a line limit at all.
	*/
	short int m_maxLines;
	/// ellipsis style
	EllipsizeMode m_ellipsize;
	/// maximum width, valid values are 0 to 100 (percent of screen).
	/// 0 means no limit, and in practice it will turn off wrapping of text.
	/// 96 (being used as the default), in practice means wrap or ellipsize at the edge of the screen minus at least a 2% margin on each side.
	unsigned short int m_maxWidth;
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
	Dimensions& dimensions() { return m_texture.dimensions; }
	/// @return number of lines rendered.
	size_t lines() { return m_lines; }

private:
	size_t m_lines = 1;
	double m_x;
	double m_y;
	Texture m_texture;
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
	/// Returns the number of lines in a contained OpenGLText.
	size_t totalLines() { return m_opengl_text->lines(); }

private:
	std::unique_ptr<OpenGLText> m_opengl_text;
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
	void draw(std::vector<TZoomText>& _text, bool padSyllables = false);
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
	/// Returns the maximum number of lines in a contained OpenGLText.
	size_t totalLines();
	
private:
	std::vector<std::unique_ptr<OpenGLText>> m_opengl_text;
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

