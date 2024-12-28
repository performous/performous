#pragma once

#include "color.hh"
#include "texture.hh"
#include "unicode.hh"
#include "graphic/size.hh"
#include <memory>
#include <string>
#include <vector>

/// Load custom fonts from current theme and data folders
void loadFonts();

/// zoomed text
struct TZoomText {
	/// text
	std::string string;
	/// zoomfactor
	float factor;
	/// constructor
	TZoomText(std::string const& str = std::string()): string(str), factor(1.0f) {}
};

/// Special theme for creating opengl themed surfaces
/** this structure does not include:
 *  - library specific field
 *  - global positionning
 * the font{family,style,weight,align} are the one found into SVGs
 */
struct TextStyle {
	cairo_line_join_t LineJoin() const {	///< convert svg string to cairo enum.
		if (UnicodeUtil::caseEqual(stroke_linejoin, "round")) return CAIRO_LINE_JOIN_ROUND;
		if (UnicodeUtil::caseEqual(stroke_linejoin, "bevel")) return CAIRO_LINE_JOIN_BEVEL;
		return CAIRO_LINE_JOIN_MITER;
	};
	cairo_line_cap_t LineCap() const {	///< convert svg string to cairo enum.
		if (UnicodeUtil::caseEqual(stroke_linecap, "round")) return CAIRO_LINE_CAP_ROUND;
		if (UnicodeUtil::caseEqual(stroke_linecap, "square")) return CAIRO_LINE_CAP_SQUARE;
		return CAIRO_LINE_CAP_BUTT;
	};
	Color fill_col; ///< fill color
	Color stroke_col; ///< stroke color
	float stroke_width = 0.f; ///< stroke thickness
	float stroke_miterlimit = 1.0f; ///< stroke miter limit
	float fontsize = 16.f; ///< fontsize
	std::string fontfamily; ///< fontfamily
	std::string fontstyle; ///< fontstyle
	std::string fontweight; ///< fontweight
	std::string fontalign; ///< alignment
	std::string	stroke_linejoin; ///< stroke line-join type
	std::string	stroke_linecap; ///< stroke line-join type
	bool stroke_paintfirst; ///< paint the stroke, or the fill first
};

/// this class will enable to create a texture from a themed text structure
/** it will not cache any data (class using this class should)
 * it provides size of the texture are drawn (x,y)
 * it provides size of the texture created (x_power_of_two, y_power_of_two)
 */
class OpenGLText {
public:
	OpenGLText(std::string const& text, std::unique_ptr<Texture>&, float width, float height);
	OpenGLText(OpenGLText&&);

	OpenGLText& operator=(OpenGLText&&);

	/// draws area
	void draw(Window&, Dimensions &_dim, TexCoords &_tex);
	/// draws full texture
	void draw(Window&);
	float getWidth() const { return m_width; }
	float getHeight() const { return m_height; }
	/// @returns dimension of texture
	Dimensions& dimensions() { return m_texture->dimensions; }

private:
	std::string m_text;
	std::unique_ptr<Texture> m_texture;
	float m_width;
	float m_height;
};

/// themed svg texts (simple)
class SvgTxtThemeSimple {
public:
	SvgTxtThemeSimple(fs::path const& themeFile, float factor = 1.0f);
	/// renders text
	void render(std::string const& text);
	/// draws texture
	void draw(Window&);
	/// gets dimensions
	Dimensions& dimensions() { return m_opengl_text->dimensions(); }

private:
	std::unique_ptr<OpenGLText> m_opengl_text;
	std::string m_cache_text;
	TextStyle m_text;
	float m_factor;
};

class SvgTxtRect
{
    public:
    // specifically not using m_ syntax for the sake of brevity, e.g.:  m_rect.m_x
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};
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
	/// horizontal align
	enum class Align { A_ASIS, LEFT, CENTER, RIGHT };
	/// order of text-painting operations
	enum class PaintOrder { FILL_FIRST, STROKE_FIRST, MARKERS_FIRST }; // No support for markers at the moment
	/// dimensions, what else
	Dimensions dimensions;

	SvgTxtTheme(fs::path const& themeFile, float factor = 1.0f);
	/// draws text with alpha
	void draw(Window&, std::vector<TZoomText>& _text, bool lyrics = false);
	/// draw text with alpha
	void draw(Window&, std::string _text);
	Size measure(std::string const& text);
	/// sets highlight
	void setHighlight(fs::path const& themeFile);
	/// width
	float getWidth() const { return m_texture_width; }
	float w() const { return m_texture_width; }
	/// height
	float getHeight() const { return m_texture_height; }
	float h() const { return m_texture_height; }
	/// set align
	void setAlign(Align align) { m_align = align; }

private:
	std::vector<std::unique_ptr<OpenGLText>> m_opengl_text;
	Align m_align;
	PaintOrder m_paintorder{PaintOrder::FILL_FIRST}; // Fill, then Stroke is SVG default
	//float m_x;
	//float m_y;
	//float m_width;
	//float m_height;
    SvgTxtRect m_rect;
	float m_factor;
	float m_texture_width;
	float m_texture_height;
	std::string m_cache_text;
	TextStyle m_textstyle;
	TextStyle m_textstyle_highlight;
};
