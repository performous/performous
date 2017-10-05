#include "opengl_text.hh"

#include <boost/lexical_cast.hpp>
#include "fontconfig/fontconfig.h"
#include <libxml++/libxml++.h>
#include <pango/pangocairo.h>
#include <cmath>
#include <iostream>
#include <sstream>
#include "fs.hh"

void loadFonts() {
	FcConfig *config = FcInitLoadConfig();
	for (fs::path const& font: listFiles("fonts")) {
		FcBool err = FcConfigAppFontAddFile(config, reinterpret_cast<const FcChar8*>(font.string().c_str()));
		std::clog << "font/info: Loading font " << font << ": " << ((err == FcTrue)?"ok":"error") << std::endl;
	}
	if (!FcConfigBuildFonts(config))
		throw std::logic_error("Could not build font database.");
	FcConfigSetCurrent(config);

	// This would all be very useless if pango+cairo didn't use the fontconfig+freetype backend:

	PangoCairoFontMap *map = PANGO_CAIRO_FONT_MAP(pango_cairo_font_map_get_default());
	std::clog << "font/info: PangoCairo is using font map " << G_OBJECT_TYPE_NAME(map) << std::endl;
	if (pango_cairo_font_map_get_font_type(map) != CAIRO_FONT_TYPE_FT) {
		PangoCairoFontMap *ftMap = PANGO_CAIRO_FONT_MAP(pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT));
		std::clog << "font/info: Switching to font map " << G_OBJECT_TYPE_NAME(ftMap) << std::endl;
		if (ftMap)
			pango_cairo_font_map_set_default(ftMap);
		else
			std::clog << "font/error: Can't switch to FreeType, fonts will be unavailable!" << std::endl;
	}
}

namespace {
	PangoAlignment parseAlignment(std::string const& fontalign) {
		if (fontalign == "start") return PANGO_ALIGN_LEFT;
		if (fontalign == "center" || fontalign == "middle") return PANGO_ALIGN_CENTER;
		if (fontalign == "end") return PANGO_ALIGN_RIGHT;
		throw std::logic_error(fontalign + ": Unknown font alignment (opengl_text.cc)");
	}

	PangoWeight parseWeight(std::string const& fontweight) {
		if (fontweight == "normal") return PANGO_WEIGHT_NORMAL;
		if (fontweight == "bold") return PANGO_WEIGHT_BOLD;
		if (fontweight == "bolder") return PANGO_WEIGHT_ULTRABOLD;
		throw std::logic_error(fontweight + ": Unknown font weight (opengl_text.cc)");
	}

	PangoStyle parseStyle(std::string const& fontstyle) {
		if (fontstyle == "normal") return PANGO_STYLE_NORMAL;
		if (fontstyle == "italic") return PANGO_STYLE_ITALIC;
		if (fontstyle == "oblique") return PANGO_STYLE_OBLIQUE;
		throw std::logic_error(fontstyle + ": Unknown font style (opengl_text.cc)");
	}
}

OpenGLText::OpenGLText(TextStyle& _text, double m) {
	m *= 2.0;  // HACK to improve text quality without affecting compatibility with old versions
	// Setup font settings
	PangoAlignment alignment = parseAlignment(_text.fontalign);
	std::shared_ptr<PangoFontDescription> desc(
	  pango_font_description_new(),
	  pango_font_description_free);
	pango_font_description_set_weight(desc.get(), parseWeight(_text.fontweight));
	pango_font_description_set_style(desc.get(), parseStyle(_text.fontstyle));
	pango_font_description_set_family(desc.get(), _text.fontfamily.c_str());
	pango_font_description_set_absolute_size(desc.get(), _text.fontsize * PANGO_SCALE * m);
	double border = _text.stroke_width * m;
	// Setup Pango context and layout
	std::shared_ptr<PangoContext> ctx(
	  pango_font_map_create_context(pango_cairo_font_map_get_default()),
	  g_object_unref);
	std::shared_ptr<PangoLayout> layout(
	  pango_layout_new(ctx.get()),
	  g_object_unref);
	pango_layout_set_alignment(layout.get(), alignment);
	pango_layout_set_font_description(layout.get(), desc.get());
	pango_layout_set_text(layout.get(), _text.text.c_str(), -1);
	// Compute text extents
	{
		PangoRectangle rec;
		pango_layout_get_pixel_extents(layout.get(), NULL, &rec);
		m_x = rec.width + border;  // Add twice half a border for margins
		m_y = rec.height + border;
	}
	// Create Cairo surface and drawing context
	std::shared_ptr<cairo_surface_t> surface(
	  cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_x, m_y),
	  cairo_surface_destroy);
	std::shared_ptr<cairo_t> dc(
	  cairo_create(surface.get()),
	  cairo_destroy);
	// Keep things sharp and fast, we scale with OpenGL anyway...
	cairo_set_antialias(dc.get(), CAIRO_ANTIALIAS_NONE);
	// Add Pango line and path to proper position on the DC
	cairo_move_to(dc.get(), 0.5 * border, 0.5 * border);  // Margins needed for border stroke to fit in
	pango_cairo_update_layout(dc.get(), layout.get());
	pango_cairo_layout_path(dc.get(), layout.get());
	// Render text
	if (_text.fill_col.a > 0.0) {
		cairo_set_source_rgba(dc.get(), _text.fill_col.r, _text.fill_col.g, _text.fill_col.b, _text.fill_col.a);
		cairo_fill_preserve(dc.get());
	}
	// Render text border
	if (_text.stroke_col.a > 0.0) {
		cairo_set_line_width(dc.get(), border);
		cairo_set_source_rgba(dc.get(), _text.stroke_col.r, _text.stroke_col.g, _text.stroke_col.b, _text.stroke_col.a);
		cairo_stroke(dc.get());
	}
	// Load into m_surface (OpenGL texture)
	Bitmap bitmap(cairo_image_surface_get_data(surface.get()));
	bitmap.fmt = pix::INT_ARGB;
	bitmap.linearPremul = true;
	bitmap.resize(cairo_image_surface_get_width(surface.get()), cairo_image_surface_get_height(surface.get()));
	m_surface.load(bitmap);
	// We don't want text quality multiplier m to affect rendering size...
	m_x /= m;
	m_y /= m;
}

void OpenGLText::draw() {
	m_surface.draw();
}

void OpenGLText::draw(Dimensions &_dim, TexCoords &_tex) {
	m_surface.dimensions = _dim;
	m_surface.tex = _tex;
	m_surface.draw();
}

namespace {
	void parseTheme(fs::path const& themeFile, TextStyle &_theme, double &_width, double &_height, double &_x, double &_y, SvgTxtTheme::Align& _align) {
		xmlpp::Node::PrefixNsMap nsmap;
		nsmap["svg"] = "http://www.w3.org/2000/svg";
		xmlpp::DomParser dom(themeFile.string());
		xmlpp::NodeSet n;
		// Parse width attribute
		n = dom.get_document()->get_root_node()->find("/svg:svg/@width",nsmap);
		if (n.empty()) throw std::runtime_error("Unable to find text theme info width in "+themeFile.string());
		xmlpp::Attribute& width = dynamic_cast<xmlpp::Attribute&>(*n[0]);
		_width = boost::lexical_cast<double>(width.get_value());
		// Parse height attribute
		n = dom.get_document()->get_root_node()->find("/svg:svg/@height",nsmap);
		if (n.empty()) throw std::runtime_error("Unable to find text theme info height in "+themeFile.string());
		xmlpp::Attribute& height = dynamic_cast<xmlpp::Attribute&>(*n[0]);
		_height = boost::lexical_cast<double>(height.get_value());
		// Parse text style attribute (CSS rules)
		n = dom.get_document()->get_root_node()->find("/svg:svg//svg:text/@style",nsmap);
		if (n.empty()) throw std::runtime_error("Unable to find text theme info style in "+themeFile.string());
		xmlpp::Attribute& style = dynamic_cast<xmlpp::Attribute&>(*n[0]);
		std::istringstream iss(style.get_value());
		std::string token;
		while (std::getline(iss, token, ';')) {
			std::istringstream iss2(token);
			std::getline(iss2, token, ':');
			if (token == "font-size") {
				// Parse as int because https://llvm.org/bugs/show_bug.cgi?id=17782
				int value;
				iss2 >> value;
				_theme.fontsize = value;
			}
			else if (token == "font-family") std::getline(iss2, _theme.fontfamily);
			else if (token == "font-style") std::getline(iss2, _theme.fontstyle);
			else if (token == "font-weight") std::getline(iss2, _theme.fontweight);
			else if (token == "stroke-width") iss2 >> _theme.stroke_width;
			else if (token == "stroke-opacity") iss2 >> _theme.stroke_col.a;
			else if (token == "fill-opacity") iss2 >> _theme.fill_col.a;
			else if (token == "fill") iss2 >> _theme.fill_col;
			else if (token == "stroke") iss2 >> _theme.stroke_col;
			else if (token == "text-anchor") {
				std::string value;
				std::getline(iss2, value);
				_theme.fontalign = value;
				if (value == "start") _align = SvgTxtTheme::LEFT;
				else if (value == "middle") _align = SvgTxtTheme::CENTER;
				else if (value == "end") _align = SvgTxtTheme::RIGHT;
			}
		}
		// Parse x and y attributes
		n = dom.get_document()->get_root_node()->find("/svg:svg//svg:text/@x",nsmap);
		if (n.empty()) throw std::runtime_error("Unable to find text theme info x in "+themeFile.string());
		xmlpp::Attribute& x = dynamic_cast<xmlpp::Attribute&>(*n[0]);
		_x = boost::lexical_cast<double>(x.get_value());
		n = dom.get_document()->get_root_node()->find("/svg:svg//svg:text/@y",nsmap);
		if (n.empty()) throw std::runtime_error("Unable to find text theme info y in "+themeFile.string());
		xmlpp::Attribute& y = dynamic_cast<xmlpp::Attribute&>(*n[0]);
		_y = boost::lexical_cast<double>(y.get_value());
	}
}

SvgTxtThemeSimple::SvgTxtThemeSimple(fs::path const& themeFile, double factor) : m_factor(factor) {
	SvgTxtTheme::Align a;
	double tmp;
	parseTheme(themeFile, m_text, tmp, tmp, tmp, tmp, a);
}

void SvgTxtThemeSimple::render(std::string _text) {
	if (!m_opengl_text.get() || m_cache_text != _text) {
		m_cache_text = _text;
		m_text.text = _text;
		m_opengl_text.reset(new OpenGLText(m_text, m_factor));
	}
}

void SvgTxtThemeSimple::draw() {
	m_opengl_text->draw();
}

SvgTxtTheme::SvgTxtTheme(fs::path const& themeFile, double factor): m_align(),m_factor(factor) {
	parseTheme(themeFile, m_text, m_width, m_height, m_x, m_y, m_align);
	dimensions.stretch(0.0, 0.0).middle(-0.5 + m_x / m_width).center((m_y - 0.5 * m_height) / m_width);
}

void SvgTxtTheme::setHighlight(fs::path const& themeFile) {
	double a,b,c,d;
	Align e;
	parseTheme(themeFile, m_text_highlight, a, b, c, d, e);
}

// void SvgTxtTheme::draw(std::vector<std::string> const& _text) {
// 	std::vector<TZoomText> tmp;
// 	
// 	for (auto const& str: _text) {		
// 		TZoomText t;
// 		t.string = str;
// 		t.factor = 1.0;
// 		
// 		tmp.push_back(t);
// 	}
// 	std::clog << "text/debug: This function did get called." << std::endl;
// 	draw(tmp);
// }

void SvgTxtTheme::draw(std::string _text) {
	std::vector<TZoomText> tmp;
	TZoomText t;
	t.string = _text;
	t.factor = 1.0;
	tmp.push_back(t);
	draw(tmp);
}

void SvgTxtTheme::draw(std::vector<TZoomText>& _text) {
	std::string tmp;

	for (auto& zt: _text) { 
		bool wordBoundaryCheck = false;
	// Check whether current syllable begins or previous syllable ends with a space.
		if (_text.size() > 1) {
			wordBoundaryCheck = (zt.string.front() == ' ');
			if (wordBoundaryCheck == false && tmp.size() > 0) {
				wordBoundaryCheck = (tmp.back() == ' ');
			}
		zt.updateWordStart(wordBoundaryCheck);	
		}
		tmp += zt.string;
	}

	if (m_opengl_text.size() != _text.size() || m_cache_text != tmp) {
		m_cache_text = tmp;
		m_opengl_text.clear();
		for (auto& zt: _text) {
			if (zt.wordStart == true) { zt.addSpace(); } // If it's a different word, add an extra space to account for lyrics zooming in.
			m_text.text = zt.string;
			m_opengl_text.push_back(new OpenGLText(m_text, m_factor));
		}
	}
	double text_x = 0.0;
	double text_y = 0.0;
	// First compute maximum height and whole length
	for (unsigned int i = 0; i < _text.size(); i++ ) {
		text_x += m_opengl_text[i].x();
		text_y = std::max(text_y, m_opengl_text[i].y());
	}

	double texture_ar = text_x / text_y;
	m_texture_width = std::min(0.96, text_x / targetWidth); // targetWidth is defined in video_driver.cc, it's the base rendering width, used to project the svg onto a gltexture. currently we're targeting 1366x768 as base resolution.
	
	double position_x = dimensions.x1();
	if (m_align == CENTER) position_x -= 0.5 * m_texture_width;
	if (m_align == RIGHT) position_x -= m_texture_width;

	if ((position_x + m_texture_width) > 0.48) { 
		m_texture_width = (0.48 - position_x);
	}
	m_texture_height = m_texture_width / texture_ar; // Keep aspect ratio.
	for (unsigned int i = 0; i < _text.size(); i++ ) {
		double syllable_x = m_opengl_text[i].x();
		double syllable_width = syllable_x *  m_texture_width / text_x;
		double syllable_height = m_texture_height;
		double syllable_ar = syllable_width / syllable_height;
		Dimensions dim(syllable_ar);
		dim.fixedHeight(m_texture_height).center(dimensions.y1());
		dim.middle(position_x + 0.5 * dim.w());
		TexCoords tex;
		double factor = _text[i].factor;
		if (factor > 1.0) {
			ColorTrans c(Color(m_text_highlight.fill_col.r, m_text_highlight.fill_col.g, m_text_highlight.fill_col.b));
			dim.fixedWidth(dim.w() * factor);
			m_opengl_text[i].draw(dim, tex);
		} 
		else { m_opengl_text[i].draw(dim, tex); }
		position_x += syllable_width;
	}
}

