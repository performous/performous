#include "opengl_text.hh"

#include <boost/lexical_cast.hpp>
#include <libxml++/libxml++.h>
#include <pango/pangocairo.h>
#include <cmath>
#include <iostream>
#include <sstream>

OpenGLText::OpenGLText(TThemeTxtOpenGL& _text, double m) {
	if (_text.fontfamily.empty()) _text.fontfamily = "Arial";

	PangoAlignment alignment = PANGO_ALIGN_LEFT;
	if (_text.fontalign == "start") alignment = PANGO_ALIGN_LEFT;
	else if (_text.fontalign == "center") alignment = PANGO_ALIGN_CENTER;
	else if (_text.fontalign == "end") alignment = PANGO_ALIGN_RIGHT;

	PangoWeight weight = PANGO_WEIGHT_NORMAL;
	if (_text.fontweight == "normal") weight = PANGO_WEIGHT_NORMAL;
	else if (_text.fontweight == "bold") weight = PANGO_WEIGHT_BOLD;
	else if (_text.fontweight == "bolder") weight = PANGO_WEIGHT_ULTRABOLD;

	PangoStyle style = PANGO_STYLE_NORMAL;
	if (_text.fontstyle == "normal") style = PANGO_STYLE_NORMAL;
	else if (_text.fontstyle == "italic") style = PANGO_STYLE_ITALIC;
	else if (_text.fontstyle == "oblique") style = PANGO_STYLE_OBLIQUE;

	// set font description
	PangoFontDescription *desc = pango_font_description_new();
	pango_font_description_set_weight(desc, weight);
	pango_font_description_set_style(desc, style);
	pango_font_description_set_family(desc, _text.fontfamily.c_str());
	pango_font_description_set_absolute_size(desc, _text.fontsize * PANGO_SCALE * m);

	double border = _text.stroke_width * m;
	double margin = 2.0 * border;

	// compute text extents
	{
		PangoContext* ctx = pango_cairo_font_map_create_context ((PangoCairoFontMap*)pango_cairo_font_map_get_default());
		PangoLayout* layout = pango_layout_new(ctx);
		pango_layout_set_alignment(layout, alignment);
		pango_layout_set_font_description (layout, desc);
		pango_layout_set_text (layout, _text.text.c_str(), -1);
		PangoRectangle rec1, rec2;
		pango_layout_get_pixel_extents (layout,&rec1,&rec2);
		m_x = rec2.width + 2.0 * margin;
		m_y = rec2.height + 2.0 * margin;
		m_x_advance = rec1.x;
		m_y_advance = rec1.y;
		g_object_unref (layout);
		g_object_unref (ctx);
	}

	// create surface
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_x, m_y);
	cairo_t *dc = cairo_create(surface);

	// draw the surface
	PangoLayout* layout = pango_cairo_create_layout(dc);
	pango_layout_set_alignment(layout, alignment);
	pango_layout_set_font_description(layout, desc);
	pango_layout_set_text(layout, _text.text.c_str(), -1);
	cairo_save(dc);
	cairo_move_to(dc, margin, margin);
	pango_cairo_show_layout (dc, layout);
	pango_cairo_layout_path(dc, layout);
	if (_text.fill_col.a > 0.0) {
		cairo_set_source_rgba(dc, _text.fill_col.r, _text.fill_col.g, _text.fill_col.b, _text.fill_col.a);
		cairo_fill_preserve(dc);
	}
	if (_text.stroke_col.a > 0.0) {
		cairo_set_line_width(dc, border);
		cairo_set_source_rgba(dc, _text.stroke_col.r, _text.stroke_col.g, _text.stroke_col.b, _text.stroke_col.a);
		cairo_stroke(dc);
	}
	pango_cairo_update_layout (dc, layout);
	cairo_restore(dc);
	g_object_unref(layout);

	m_surface.load(cairo_image_surface_get_width(surface), cairo_image_surface_get_height(surface), pix::INT_ARGB, cairo_image_surface_get_data(surface));

	// delete surface
	cairo_destroy(dc);
	cairo_surface_destroy(surface);
	// delete the font description
	pango_font_description_free(desc);
	m_x /= m;
	m_y /= m;
	m_x_advance /= m;
	m_y_advance /= m;
}

void OpenGLText::draw() {
	m_surface.draw();
}

void OpenGLText::draw(Dimensions &_dim, TexCoords &_tex) {
	m_surface.dimensions = _dim;
	m_surface.tex = _tex;
	m_surface.draw();
}

void parseTheme( std::string _theme_file, TThemeTxtOpenGL &_theme, double &_width, double &_height, double &_x, double &_y, SvgTxtTheme::Align& _align) {
	// this should stay here for the moment
	_theme.fontalign = "center";

	xmlpp::Node::PrefixNsMap nsmap;
	nsmap["svg"] = "http://www.w3.org/2000/svg";
	xmlpp::DomParser dom(_theme_file);
	xmlpp::NodeSet n;

	n = dom.get_document()->get_root_node()->find("/svg:svg/@width",nsmap);
	if (n.empty()) throw std::runtime_error("Unable to find text theme info width in "+_theme_file);
	xmlpp::Attribute& width = dynamic_cast<xmlpp::Attribute&>(*n[0]);
	_width = boost::lexical_cast<double>(width.get_value());
	n = dom.get_document()->get_root_node()->find("/svg:svg/@height",nsmap);
	if (n.empty()) throw std::runtime_error("Unable to find text theme info height in "+_theme_file);
	xmlpp::Attribute& height = dynamic_cast<xmlpp::Attribute&>(*n[0]);
	_height = boost::lexical_cast<double>(height.get_value());

	n = dom.get_document()->get_root_node()->find("/svg:svg//svg:text/@style",nsmap);
	if (n.empty()) throw std::runtime_error("Unable to find text theme info style in "+_theme_file);
	xmlpp::Attribute& style = dynamic_cast<xmlpp::Attribute&>(*n[0]);
	std::istringstream iss(style.get_value());
	std::string token;
	while (std::getline(iss, token, ';')) {
		std::istringstream iss2(token);
		std::getline(iss2, token, ':');
		if (token == "font-size") iss2 >> _theme.fontsize;
		else if (token == "font-family") std::getline(iss2, _theme.fontfamily);
		else if (token == "font-style") std::getline(iss2, _theme.fontstyle);
		else if (token == "font-weight") std::getline(iss2, _theme.fontweight);
		else if (token == "stroke-width") iss2 >> _theme.stroke_width;
		else if (token == "stroke-opacity") iss2 >> _theme.stroke_col.a;
		else if (token == "fill-opacity") iss2 >> _theme.fill_col.a;
		else if (token == "fill") _theme.fill_col = getColor(iss2);
		else if (token == "stroke") _theme.stroke_col = getColor(iss2);
		else if (token == "text-anchor") {
			std::string value;
			std::getline(iss2, value);
			if (value == "start") _align = SvgTxtTheme::LEFT;
			else if (value == "middle") _align = SvgTxtTheme::CENTER;
			else if (value == "end") _align = SvgTxtTheme::RIGHT;
		}
	}

	n = dom.get_document()->get_root_node()->find("/svg:svg//svg:text/@x",nsmap);
	if (n.empty()) throw std::runtime_error("Unable to find text theme info x in "+_theme_file);
	xmlpp::Attribute& x = dynamic_cast<xmlpp::Attribute&>(*n[0]);
	_x = boost::lexical_cast<double>(x.get_value());
	n = dom.get_document()->get_root_node()->find("/svg:svg//svg:text/@y",nsmap);
	if (n.empty()) throw std::runtime_error("Unable to find text theme info y in "+_theme_file);
	xmlpp::Attribute& y = dynamic_cast<xmlpp::Attribute&>(*n[0]);
	_y = boost::lexical_cast<double>(y.get_value());
}

SvgTxtThemeSimple::SvgTxtThemeSimple(std::string _theme_file, double factor) : m_factor(factor) {
	SvgTxtTheme::Align a;
	parseTheme(_theme_file, m_text, m_width, m_height, m_x, m_y, a);
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

SvgTxtTheme::SvgTxtTheme(std::string _theme_file, double factor): m_align(),m_factor(factor) {
	parseTheme(_theme_file, m_text, m_width, m_height, m_x, m_y, m_align);
	dimensions.stretch(0.0, 0.0).middle(-0.5 + m_x / m_width).center((m_y - 0.5 * m_height) / m_width);
}

void SvgTxtTheme::setHighlight(std::string _theme_file) {
	double a,b,c,d;
	Align e;
	parseTheme(_theme_file, m_text_highlight, a, b, c, d, e);
}

void SvgTxtTheme::draw(std::vector<std::string> const& _text) {
	std::vector<TZoomText> tmp;

	for (std::vector<std::string>::const_iterator it = _text.begin(); it != _text.end(); ++it) {
		TZoomText t;
		t.string = *it;
		t.factor = 1.0;
		tmp.push_back(t);
	}

	draw(tmp);
}

void SvgTxtTheme::draw(std::string _text) {
	std::vector<TZoomText> tmp;
	TZoomText t;
	t.string = _text;
	t.factor = 1.0;
	tmp.push_back(t);
	draw(tmp);
}

void SvgTxtTheme::draw(std::vector<TZoomText> const& _text) {
	std::string tmp;
	for (unsigned int i = 0 ; i < _text.size(); i++ ) tmp += _text[i].string;

	if (m_opengl_text.size() != _text.size() || m_cache_text != tmp) {
		m_cache_text = tmp;
		m_opengl_text.clear();
		for (unsigned int i = 0; i < _text.size(); i++ ) {
			m_text.text = _text[i].string;
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
	m_texture_width = std::min(0.96, text_x/800.);
	m_texture_height = m_texture_width / texture_ar;

	double position_x = dimensions.x1();
	if (m_align == CENTER) position_x -= 0.5 * m_texture_width;
	if (m_align == RIGHT) position_x -= m_texture_width;

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
		} else m_opengl_text[i].draw(dim, tex);
		position_x += syllable_width;
	}
}

