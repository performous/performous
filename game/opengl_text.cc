#include "opengl_text.hh"

#include "libxml++-impl.hh"

#include <cstdint>
#include <cmath>
#include <iostream>
#include <sstream>
#include "fs.hh"
#include "graphic/text_renderer.hh"
#include "fontconfig/fontconfig.h"
#include <pango/pangocairo.h>

void loadFonts() {
	auto config = std::unique_ptr<FcConfig, decltype(&FcConfigDestroy)>(FcInitLoadConfig(), &FcConfigDestroy);
	for (fs::path const& font: listFiles("fonts")) {
		FcBool err = FcConfigAppFontAddFile(config.get(), reinterpret_cast<const FcChar8*>(font.string().c_str()));
		std::clog << "font/info: Loading font " << font << ": " << ((err == FcTrue)?"ok":"error") << std::endl;
	}
	if (!FcConfigBuildFonts(config.get()))
		throw std::logic_error("Could not build font database.");

		// FcConfigSetCurrent increments the refcount of config, thus the local handle on config can be deleted safely.
	FcConfigSetCurrent(config.get());

	// This would all be very useless if pango+cairo didn't use the fontconfig+freetype backend:

	PangoCairoFontMap *map = PANGO_CAIRO_FONT_MAP(pango_cairo_font_map_get_default());
	std::clog << "font/info: PangoCairo is using font map " << G_OBJECT_TYPE_NAME(map) << std::endl;
	if (pango_cairo_font_map_get_font_type(map) != CAIRO_FONT_TYPE_FT) {
		PangoCairoFontMap *ftMap = PANGO_CAIRO_FONT_MAP(pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT));
		if (ftMap) {
			std::clog << "font/info: Switching to font map " << G_OBJECT_TYPE_NAME(ftMap) << std::endl;
			pango_cairo_font_map_set_default(ftMap);
		} else
			std::clog << "font/error: Can't switch to FreeType, fonts will be unavailable!" << std::endl;
	}
}

OpenGLText::OpenGLText(std::string const& text, std::unique_ptr<Texture>& texture, float width, float height)
: m_text(text), m_texture(std::move(texture)), m_width(width), m_height(height) {
}

OpenGLText::OpenGLText(OpenGLText&& other)
: m_text(std::move(other.m_text)), m_texture(std::move(other.m_texture)), m_width(other.m_width), m_height(other.m_height) {
	other.m_width = other.m_height = 0.f;
}

OpenGLText& OpenGLText::operator=(OpenGLText&& other) {
	m_text = std::move(other.m_text);
	m_texture = std::move(other.m_texture);
	m_width = other.m_width;
	m_height = other.m_height;

	other.m_width = other.m_height = 0.f;

	return *this;
}

void OpenGLText::draw(Window& window) {
	m_texture->draw(window);
}

void OpenGLText::draw(Window& window, Dimensions &_dim, TexCoords &_tex) {
	m_texture->dimensions = _dim;
	m_texture->tex = _tex;
	m_texture->draw(window);
}

namespace {
	void parseTheme(fs::path const& themeFile, TextStyle &_theme, float &_width, float &_height, float &_x, float &_y, SvgTxtTheme::Align& _align) {
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
				_theme.fontsize = static_cast<float>(value);
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
				if (value == "start") _align = SvgTxtTheme::Align::LEFT;
				else if (value == "middle") _align = SvgTxtTheme::Align::CENTER;
				else if (value == "end") _align = SvgTxtTheme::Align::RIGHT;
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

SvgTxtThemeSimple::SvgTxtThemeSimple(fs::path const& themeFile, float factor) : m_factor(factor) {
	SvgTxtTheme::Align a;
	float tmp;
	parseTheme(themeFile, m_text, tmp, tmp, tmp, tmp, a);
}

void SvgTxtThemeSimple::render(std::string const& text) {
	if (!m_opengl_text.get() || m_cache_text != text) {
		m_cache_text = text;

		static TextRenderer renderer;

		m_opengl_text = std::make_unique<OpenGLText>(renderer.render(text, m_text, m_factor));
	}
}

void SvgTxtThemeSimple::draw(Window& window) {
	m_opengl_text->draw(window);
}

SvgTxtTheme::SvgTxtTheme(fs::path const& themeFile, float factor): m_align(), m_factor(factor) {
	parseTheme(themeFile, m_textstyle, m_width, m_height, m_x, m_y, m_align);
	dimensions.stretch(0.0f, 0.0f).middle(-0.5f + m_x / m_width).center((m_y - 0.5f * m_height) / m_width);
}

void SvgTxtTheme::setHighlight(fs::path const& themeFile) {
	float a,b,c,d;
	Align e;
	parseTheme(themeFile, m_textstyle_highlight, a, b, c, d, e);
}

void SvgTxtTheme::draw(Window& window, std::string _text) {
	std::vector<TZoomText> tmp;
	TZoomText t;
	t.string = _text;
	t.factor = 1.0f;
	tmp.push_back(t);
	draw(window, tmp);
}

void SvgTxtTheme::draw(Window& window, std::vector<TZoomText>& _text, bool lyrics) {
	std::string tmp;

	for (auto& zt: _text) {
		tmp += zt.string;
	}

	if (m_opengl_text.size() != _text.size() || m_cache_text != tmp) {
		m_cache_text = tmp;
		m_opengl_text.clear();
		auto renderer = TextRenderer();
		for (const auto& zt: _text) {
			m_opengl_text.emplace_back(std::make_unique<OpenGLText>(renderer.render(zt.string, m_textstyle, m_factor)));
		}
	}
	float text_x = 0.0f;
	float text_y = 0.0f;
	// First compute maximum height and whole length
	for (size_t i = 0; i < _text.size(); i++ ) {
		text_x += m_opengl_text[i]->getWidth();
		text_y = std::max(text_y, m_opengl_text[i]->getHeight());
	}

	float texture_ar = text_x / text_y;
	m_texture_width = std::min(0.96f, text_x / targetWidth); // targetWidth is defined in video_driver.cc, it's the base rendering width, used to project the svg onto a gltexture. currently we're targeting 1366x768 as base resolution.
	float position_x = dimensions.x1();

	if (m_align == Align::CENTER) position_x -= 0.5f * m_texture_width;
	if (m_align == Align::RIGHT) position_x -= m_texture_width;

	if ((position_x + m_texture_width > 0.48f)) {
		m_texture_width = (0.48f - position_x) ;
	}
	m_texture_height = m_texture_width / texture_ar; // Keep aspect ratio.
	for (size_t i = 0; i < _text.size(); i++) {
		float syllable_x = m_opengl_text[i]->getWidth();
		float syllable_width = syllable_x *  m_texture_width / text_x * _text[i].factor;
		float syllable_height = m_texture_height * _text[i].factor;
		float syllable_ar = syllable_width / syllable_height;
		Dimensions dim(syllable_ar);
		dim.fixedHeight(m_texture_height).center(dimensions.y1());
		dim.middle(position_x + 0.5f * dim.w());
		TexCoords tex;
		float factor = _text[i].factor;
		if (factor > 1.0f) {
			LyricColorTrans lc(window, m_textstyle.fill_col, m_textstyle.stroke_col, m_textstyle_highlight.fill_col, m_textstyle_highlight.stroke_col);
			dim.fixedWidth(dim.w() * factor);
			m_opengl_text[i]->draw(window, dim, tex);
		}
		else {
			m_opengl_text[i]->draw(window, dim, tex);
		}
		position_x += (syllable_width / factor) * (lyrics ? 1.1f : 1.0f);
	}
}

Size SvgTxtTheme::measure(std::string const& text) {
	auto width = 0.0f;

	for (auto&& t : m_opengl_text)
		width += t->getWidth();

	//std::cout << "svg width: " << width << std::endl;

	return TextRenderer().measure(text, m_textstyle, m_factor) / width;
}

