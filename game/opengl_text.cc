#include "opengl_text.hh"

#include "libxml++-impl.hh"

#include "fontconfig/fontconfig.h"
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

WrappingStyle::WrappingStyle (unsigned short int _maxWidth, EllipsizeMode _ellipsize, unsigned short int _maxLines) : m_maxLines(_maxLines * -1), m_ellipsize(_ellipsize), m_maxWidth(_maxWidth > 96 ? 0 : _maxWidth) {};

OpenGLText::OpenGLText(TextStyle& _text, float m, WrappingStyle const& wrapping, Align textureAlign) {
	m *= 2.0f;  // HACK to improve text quality without affecting compatibility with old versions
	// Setup font settings
	PangoAlignment alignment = parseAlignment(_text.fontalign);
	std::shared_ptr<PangoFontDescription> desc(
	  pango_font_description_new(),
	  pango_font_description_free);
	pango_font_description_set_weight(desc.get(), parseWeight(_text.fontweight));
	pango_font_description_set_style(desc.get(), parseStyle(_text.fontstyle));
	pango_font_description_set_family(desc.get(), _text.fontfamily.c_str());
	pango_font_description_set_absolute_size(desc.get(), _text.fontsize * PANGO_SCALE * m);
	float border = _text.stroke_width * m;
	// Setup Pango context and layout
	std::shared_ptr<PangoContext> ctx(
	  pango_font_map_create_context(pango_cairo_font_map_get_default()),
	  g_object_unref);
	std::shared_ptr<PangoLayout> layout(
	  pango_layout_new(ctx.get()),
	  g_object_unref);
	pango_layout_set_wrap(layout.get(), PANGO_WRAP_WORD_CHAR);
	PangoEllipsizeMode ellipsize;
	int maxWidth = -1;
	switch (wrapping.m_ellipsize) {
		case WrappingStyle::EllipsizeMode::NONE:
			ellipsize = (wrapping.m_maxLines == -1) ? PANGO_ELLIPSIZE_NONE : PANGO_ELLIPSIZE_MIDDLE;
			break;
		case WrappingStyle::EllipsizeMode::START:
			ellipsize = PANGO_ELLIPSIZE_START;
			break;
		case WrappingStyle::EllipsizeMode::MIDDLE:
			ellipsize = PANGO_ELLIPSIZE_MIDDLE;
			break;
		case WrappingStyle::EllipsizeMode::END:
			ellipsize = PANGO_ELLIPSIZE_END;
			break;
	}
	if (wrapping.m_maxWidth != 0) { //FIXME: Got to adjust the maximum width according to the x_position.
		std::clog << "txt/debug: float targetWidth (" << std::to_string(static_cast<float>(targetWidth)) << ")" << ", float m (" << std::to_string(static_cast<float>(m)) << "), float PANGO_SCALE (" << std::to_string(static_cast<float>(PANGO_SCALE)) << ")" << std::endl;
		float width = static_cast<float>(targetWidth) * static_cast<float>(m) * static_cast<float>(PANGO_SCALE);
		std::clog << "txt/debug: float width before dividing and rounding (" << std::to_string(width) << ")" << std::endl;
		width = std::round(width * (static_cast<float>(wrapping.m_maxWidth) / 100.0));
		std::clog << "txt/debug: float width (" << std::to_string(width) << ")" << std::endl;
		maxWidth = width;
	}
	pango_layout_set_ellipsize(layout.get(), ellipsize);
	pango_layout_set_width(layout.get(), maxWidth);
	if (wrapping.m_maxLines < 0) {
		pango_layout_set_height(layout.get(), wrapping.m_maxLines);
	}
	std::clog << "txt/debug: Text " << _text.text << ", wrapping: maxLines(" << wrapping.m_maxLines << "), maxWidth(" << std::to_string(maxWidth) << "), ellipsize(" << std::to_string(ellipsize) << ")" << std::endl;
	pango_layout_set_single_paragraph_mode(layout.get(), false);
	pango_layout_set_alignment(layout.get(), alignment);
	pango_layout_set_font_description(layout.get(), desc.get());
	pango_layout_set_text(layout.get(), _text.text.c_str(), -1);
	// Compute text extents
	{
		PangoRectangle rec;
		pango_layout_get_pixel_extents(layout.get(), nullptr, &rec);
// 		std::clog << "text/debug: raw text width for : \"" << _text.text << "\", before wrapping: " << ((rec.width + border) / m) << std::endl;
// 		if ((rec.width + border) > (targetWidth * m * 0.96)) {
// 			std:: clog << "text/debug: text too long; will try again with wrapping." << std::endl;
// 			pango_layout_set_width(layout.get(), targetWidth * m * PANGO_SCALE * 0.96);	
// 			pango_layout_set_text(layout.get(), _text.text.c_str(), -1);
// 			pango_layout_get_pixel_extents(layout.get(), nullptr, &rec);
			std::clog << "text/debug: raw text width for : \"" << _text.text << "\", after wrapping: " << ((rec.width + border) / m) << std::endl;
// 		}
		m_width = rec.width + border;  // Add twice half a border for margins
		m_height = rec.height + border;
	}
	m_lines = pango_layout_get_line_count (layout.get());
	for (unsigned i = 0; i < m_lines; i++) {
	PangoLayoutLine* line = pango_layout_get_line_readonly(layout.get(), i);
	int start = line->start_index;
	int length = line->length;
	std::string lineText(pango_layout_get_text(layout.get()),start,length);
	std::clog << "text/debug: (layout) Line " << std::to_string(i + 1) << ": \"" << lineText << "\", width is: " << (m_x / m / targetWidth * 100) << "%, m_y: " << (m_y / m) << ", did we wrap?: " << std::string(pango_layout_is_wrapped(layout.get()) ? "Yes" : "No") << std::endl;
	}
	
	// Create Cairo surface and drawing context
	std::shared_ptr<cairo_surface_t> surface(
	  cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_width, m_height),
	  cairo_surface_destroy);
	std::shared_ptr<cairo_t> dc(
	  cairo_create(surface.get()),
	  cairo_destroy);
	// Keep things sharp and fast, we scale with OpenGL anyway...
	cairo_set_antialias(dc.get(), CAIRO_ANTIALIAS_FAST);
	cairo_push_group_with_content (dc.get(), CAIRO_CONTENT_COLOR_ALPHA);
	cairo_set_operator(dc.get(),CAIRO_OPERATOR_SOURCE);
	// Add Pango line and path to proper position on the DC
	cairo_move_to(dc.get(), 0.5f * border, 0.5f * border);  // Margins needed for border stroke to fit in
	pango_cairo_update_layout(dc.get(), layout.get());
	pango_cairo_layout_path(dc.get(), layout.get());
	// Render text
	if (_text.fill_col.a > 0.0f) {
		cairo_set_source_rgba(dc.get(), _text.fill_col.r, _text.fill_col.g, _text.fill_col.b, _text.fill_col.a);
		cairo_fill_preserve(dc.get());
	}
	// Render text border
	if (_text.stroke_col.a > 0.0f) {
		// Use proper line-joins and caps.
		cairo_set_line_join (dc.get(), _text.LineJoin());
		cairo_set_line_cap (dc.get(), _text.LineCap());
		cairo_set_line_join (dc.get(), _text.LineJoin());
		cairo_set_miter_limit(dc.get(), _text.stroke_miterlimit);
		cairo_set_line_width(dc.get(), border);
		cairo_set_source_rgba(dc.get(), _text.stroke_col.r, _text.stroke_col.g, _text.stroke_col.b, _text.stroke_col.a);
		cairo_stroke(dc.get());
	}
	cairo_pop_group_to_source (dc.get());
	cairo_set_operator(dc.get(),CAIRO_OPERATOR_OVER);
	cairo_paint (dc.get());
	// Load into m_texture (OpenGL texture)
	Bitmap bitmap(cairo_image_surface_get_data(surface.get()));
	bitmap.fmt = pix::INT_ARGB;
	bitmap.linearPremul = true;
	bitmap.resize(cairo_image_surface_get_width(surface.get()), cairo_image_surface_get_height(surface.get()));
	m_texture.load(bitmap, true);
	// We don't want text quality multiplier m to affect rendering size...
	m_width /= m;
	m_height /= m;
}

void OpenGLText::draw() {
	m_texture.draw();
}

void OpenGLText::draw(Dimensions &_dim, TexCoords &_tex) {
	m_texture.dimensions = _dim;
	m_texture.tex = _tex;
	m_texture.draw();
}

namespace {
	void parseTheme(fs::path const& themeFile, TextStyle &_theme, float &_width, float &_height, float &_x, float &_y, Align& _align) {
		xmlpp::Node::PrefixNsMap nsmap;
		nsmap["svg"] = "http://www.w3.org/2000/svg";
		xmlpp::DomParser dom(themeFile.string());
		// Parse width attribute
		auto n = dom.get_document()->get_root_node()->find("/svg:svg/@width",nsmap);
		if (n.empty()) throw std::runtime_error("Unable to find text theme info width in "+themeFile.string());
		xmlpp::Attribute& width = dynamic_cast<xmlpp::Attribute&>(*n[0]);
		_width = std::stof(width.get_value());
		// Parse height attribute
		n = dom.get_document()->get_root_node()->find("/svg:svg/@height",nsmap);
		if (n.empty()) throw std::runtime_error("Unable to find text theme info height in "+themeFile.string());
		xmlpp::Attribute& height = dynamic_cast<xmlpp::Attribute&>(*n[0]);
		_height = std::stof(height.get_value());
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
			else if (token == "stroke-linejoin") iss2 >> _theme.stroke_linejoin;
			else if (token == "stroke-miterlimit") iss2 >> _theme.stroke_miterlimit;
			else if (token == "stroke-linecap") iss2 >> _theme.stroke_linecap;
			else if (token == "fill-opacity") iss2 >> _theme.fill_col.a;
			else if (token == "fill") iss2 >> _theme.fill_col;
			else if (token == "stroke") iss2 >> _theme.stroke_col;
			else if (token == "text-anchor") {
				std::string value;
				std::getline(iss2, value);
				_theme.fontalign = value;
				if (value == "start") _align = Align::LEFT;
				else if (value == "middle") _align = Align::CENTER;
				else if (value == "end") _align = Align::RIGHT;
			}
		}
		// Parse x and y attributes
		n = dom.get_document()->get_root_node()->find("/svg:svg//svg:text/@x",nsmap);
		if (n.empty()) throw std::runtime_error("Unable to find text theme info x in "+themeFile.string());
		xmlpp::Attribute& x = dynamic_cast<xmlpp::Attribute&>(*n[0]);
		_x = std::stof(x.get_value());
		n = dom.get_document()->get_root_node()->find("/svg:svg//svg:text/@y",nsmap);
		if (n.empty()) throw std::runtime_error("Unable to find text theme info y in "+themeFile.string());
		xmlpp::Attribute& y = dynamic_cast<xmlpp::Attribute&>(*n[0]);
		_y = std::stof(y.get_value());
	}
}

SvgTxtThemeSimple::SvgTxtThemeSimple(fs::path const& themeFile, float factor, WrappingStyle wrapping) : m_factor(factor), m_wrapping(wrapping) {
	Align a;
	float tmp;
	parseTheme(themeFile, m_text, tmp, tmp, tmp, tmp, a);
}

void SvgTxtThemeSimple::render(std::string _text) {
	if (!m_opengl_text.get() || m_cache_text != _text) {
		m_cache_text = _text;
		m_text.text = _text;
		m_opengl_text = std::make_unique<OpenGLText>(m_text, m_factor, m_wrapping, Align());
	}
}

void SvgTxtThemeSimple::draw() {
	m_opengl_text->draw();
}

SvgTxtTheme::SvgTxtTheme(fs::path const& themeFile, float factor, WrappingStyle wrapping): m_align(), m_factor(factor), m_wrapping(wrapping) {
	parseTheme(themeFile, m_text, m_width, m_height, m_x, m_y, m_align);
	dimensions.stretch(0.0f, 0.0f).middle(-0.5f + m_x / m_width).center((m_y - 0.5 * m_height) / m_width);
}

void SvgTxtTheme::setHighlight(fs::path const& themeFile) {
	float a,b,c,d;
	Align e;
	parseTheme(themeFile, m_text_highlight, a, b, c, d, e);
}

void SvgTxtTheme::draw(std::string _text) {
	std::vector<TZoomText> tmp;
	TZoomText t;
	t.string = _text;
	t.factor = 1.0f;
	tmp.push_back(t);
	draw(tmp);
}

size_t SvgTxtTheme::totalLines() {
	size_t lines = 1;
	for (std::unique_ptr<OpenGLText> const& text: m_opengl_text) {
		size_t current_lines = text->lines();
		lines = (current_lines > lines ? current_lines : lines);
	}
	return lines;
}

void SvgTxtTheme::draw(std::vector<TZoomText>& _text, bool padSyllables) {

	std::string tmp;
	for (auto& zt: _text) { tmp += zt.string; }
	if (m_opengl_text.size() != _text.size() || m_cache_text != tmp) {
		m_cache_text = tmp;
		m_opengl_text.clear();
		for (auto& zt: _text) {
			m_text.text = zt.string;
			auto openGlPtr = std::unique_ptr<OpenGLText>(std::make_unique<OpenGLText>(m_text, m_factor, m_wrapping, m_align));
			m_opengl_text.push_back(std::move(openGlPtr));
		}
	}
	float text_x = 0.0f;
	float text_y = 0.0f;
	// First compute maximum height and whole length
	for (size_t i = 0; i < _text.size(); i++ ) {
		text_x += m_opengl_text[i]->w();
		text_y = std::max(text_y, m_opengl_text[i]->h());
	}

	float texture_ar = text_x / text_y;
	m_texture_width = std::min(padSyllables ? 0.864f : 0.96f, text_x / targetWidth); // targetWidth is defined in video_driver.cc, it's the base rendering width, used to project the svg onto a gltexture. currently we're targeting 1366x768 as base resolution.
	
	float position_x = dimensions.x1();
	std::clog << "text/debug: Initial position_x inside SvgTxtTheme::draw is: " << std::to_string(position_x) << std::endl;
	if (m_align == Align::CENTER) position_x -= 0.5f * (m_texture_width * (padSyllables ? 1.1f : 1.0f));
	if (m_align == Align::RIGHT) position_x -= m_texture_width;

	if ((position_x + m_texture_width * (padSyllables ? 1.1f : 1.0f)) > (padSyllables ? 0.432f : 0.48f)) { 
		m_texture_width = ((padSyllables ? 0.432f : 0.48f) - position_x / (padSyllables ? 1.1f : 1.0f)) ;
	}
	m_texture_height = m_texture_width / texture_ar; // Keep aspect ratio.
	for (size_t i = 0; i < _text.size(); i++) {
		float syllable_x = m_opengl_text[i]->w();
		float syllable_width = syllable_x *  m_texture_width / text_x * _text[i].factor;
		float syllable_height = m_texture_height * _text[i].factor;
		float syllable_ar = syllable_width / syllable_height;
		Dimensions dim(syllable_ar);
		dim.fixedHeight(m_texture_height).center(dimensions.y1());
		dim.middle(position_x + 0.5f * dim.w());
		TexCoords tex;
		float factor = _text[i].factor;
		std::clog << "text/debug: Text (inside draw()): \"" << m_text.text << "\", position_x: " << position_x << ",texture_height: " << m_texture_height << std::endl;
		if (factor > 1.0f) {
			LyricColorTrans lc(m_text.fill_col, m_text.stroke_col, m_text_highlight.fill_col, m_text_highlight.stroke_col);
			dim.fixedWidth(dim.w() * factor);
			m_opengl_text[i]->draw(dim, tex);
		} 
		else { m_opengl_text[i]->draw(dim, tex); }
		position_x += (syllable_width / factor) * (padSyllables ? 1.1f : 1.0f);
	}
}

