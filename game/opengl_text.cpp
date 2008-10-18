#include "opengl_text.hh"
#include <boost/lexical_cast.hpp>
#include <libxml++/libxml++.h>
#include <pango/pangocairo.h>
#include <math.h>
#include <iostream>
#include <sstream>

OpenGLText::OpenGLText(TThemeTxtOpenGL& _text) {
	PangoLayout *layout;

	if (_text.fontfamily.empty()) _text.fontfamily = "Sans";

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
	pango_font_description_set_absolute_size(desc,_text.fontsize * PANGO_SCALE);

	// compute text extents
	PangoRectangle rec1, rec2;
	PangoContext* ctx=NULL;
	ctx = pango_cairo_font_map_create_context ((PangoCairoFontMap*)pango_cairo_font_map_get_default());
	layout = pango_layout_new(ctx);
	pango_layout_set_alignment(layout, alignment);
	pango_layout_set_font_description (layout, desc);
	pango_layout_set_text (layout, _text.text.c_str(), -1);
	pango_layout_get_pixel_extents (layout,&rec1,&rec2);
	m_x = rec2.width;
	m_y = rec2.height;
	m_x_advance = rec1.x;
	m_y_advance = rec1.y;

	g_object_unref (layout);
	g_object_unref (ctx);

	// create surface
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_x, m_y);
	cairo_t *dc = cairo_create(surface);

	// draw the surface
	layout = pango_cairo_create_layout(dc);
	pango_layout_set_alignment(layout, alignment);
	pango_layout_set_font_description (layout, desc);
	pango_layout_set_text (layout, _text.text.c_str(), -1);
	cairo_save(dc);
	pango_cairo_show_layout (dc, layout);
	pango_cairo_layout_path(dc,layout);
	if (_text.fill_col.a > 0.0) {
		cairo_set_source_rgba(dc, _text.fill_col.r, _text.fill_col.g, _text.fill_col.b, _text.fill_col.a);
		if (_text.stroke_col.r != -1 && _text.stroke_col.g != -1 && _text.stroke_col.b != -1) cairo_fill_preserve(dc);
		else cairo_fill(dc);
	}
	if (_text.stroke_col.a > 0.0) {
		cairo_set_line_width(dc, _text.stroke_width);
		cairo_set_source_rgba(dc, _text.stroke_col.r, _text.stroke_col.g, _text.stroke_col.b, _text.stroke_col.a);
		cairo_stroke(dc);
	}
	pango_cairo_update_layout (dc, layout);
	cairo_restore(dc);
	g_object_unref(layout);

	m_surface.reset(new Surface(cairo_image_surface_get_width(surface), cairo_image_surface_get_height(surface), pix::INT_ARGB, cairo_image_surface_get_data(surface)));

	// delete surface
	cairo_destroy(dc);
	cairo_surface_destroy(surface);
	// delete the font description
	pango_font_description_free(desc);
}

void OpenGLText::draw(Dimensions &_dim, TexCoords &_tex) {
	m_surface->dimensions = _dim;
	m_surface->tex = _tex;
	m_surface->draw();
}

SvgTxtTheme::SvgTxtTheme(std::string _theme_file, Align _a, VAlign _v, Gravity _g, Fitting _f) : m_gravity(_g), m_fitting(_f), m_valign(_v), m_align(_a), m_cache_text("XXX FIXME: Use something else. This text must never appear in lyrics!") {
	// this should stay here for the moment
	m_text.fontalign = "center";

	xmlpp::Node::PrefixNsMap nsmap;
	nsmap["svg"] = "http://www.w3.org/2000/svg";
	xmlpp::DomParser dom(_theme_file);
	xmlpp::NodeSet n;

	n = dom.get_document()->get_root_node()->find("/svg:svg/@width",nsmap);
	if (n.empty()) throw std::runtime_error("Unable to find text theme info width in "+_theme_file);
	xmlpp::Attribute& width = dynamic_cast<xmlpp::Attribute&>(*n[0]);
	m_width = boost::lexical_cast<double>(width.get_value());
	n = dom.get_document()->get_root_node()->find("/svg:svg/@height",nsmap);
	if (n.empty()) throw std::runtime_error("Unable to find text theme info height in "+_theme_file);
	xmlpp::Attribute& height = dynamic_cast<xmlpp::Attribute&>(*n[0]);
	m_height = boost::lexical_cast<double>(height.get_value());

	n = dom.get_document()->get_root_node()->find("/svg:svg//svg:text/@style",nsmap);
	if (n.empty()) throw std::runtime_error("Unable to find text theme info style in "+_theme_file);
	xmlpp::Attribute& style = dynamic_cast<xmlpp::Attribute&>(*n[0]);
	std::istringstream iss(style.get_value());
	std::string token;
	while (std::getline(iss, token, ';')) {
		std::istringstream iss2(token);
		std::getline(iss2, token, ':');
		if (token == "font-size") iss2 >> m_text.fontsize;
		else if (token == "font-family") std::getline(iss2, m_text.fontfamily);
		else if (token == "font-style") std::getline(iss2, m_text.fontstyle);
		else if (token == "font-weight") std::getline(iss2, m_text.fontweight);
		else if (token == "stroke-width") iss2 >> m_text.stroke_width;
		else if (token == "stroke-opacity") iss2 >> m_text.stroke_col.a;
		else if (token == "fill-opacity") iss2 >> m_text.fill_col.a;
		else if (token == "fill") m_text.fill_col = getColor(iss2);
		else if (token == "stroke") m_text.stroke_col = getColor(iss2);
	}

	n = dom.get_document()->get_root_node()->find("/svg:svg//svg:text/@x",nsmap);
	if (n.empty()) throw std::runtime_error("Unable to find text theme info x in "+_theme_file);
	xmlpp::Attribute& x = dynamic_cast<xmlpp::Attribute&>(*n[0]);
	m_x = boost::lexical_cast<double>(x.get_value());
	n = dom.get_document()->get_root_node()->find("/svg:svg//svg:text/@y",nsmap);
	if (n.empty()) throw std::runtime_error("Unable to find text theme info y in "+_theme_file);
	xmlpp::Attribute& y = dynamic_cast<xmlpp::Attribute&>(*n[0]);
	m_y = boost::lexical_cast<double>(y.get_value());
};

void SvgTxtTheme::draw(std::vector<std::string> _text) {
	std::vector<TZoomText> tmp;

	for (std::vector<std::string>::iterator it = _text.begin(); it != _text.end(); ++it) {
		TZoomText t;
		t.string = *it;
		t.factor = 1.0;
		tmp.push_back(t);
	}

	draw(tmp);
}

void SvgTxtTheme::draw(std::string _text) {
	std::vector<std::string> tmp;
	tmp.push_back(_text);
	draw(tmp);
}

void SvgTxtTheme::draw(std::vector<TZoomText> _text) {
	std::string tmp;
	for (unsigned int i = 0 ; i < _text.size(); i++ ) tmp += _text[i].string;

	if (m_cache_text != tmp) {
		m_cache_text = tmp;
		for (unsigned int i = 0; i < _text.size(); i++ ) {
			m_text.text = _text[i].string;
			m_opengl_text[i].reset(new OpenGLText(m_text));
		}
	}
	double text_x = 0.0;
	double text_y = 0.0;
	// first compute maximum height and whole length
	for (unsigned int i = 0; i < _text.size(); i++ ) {
		text_x += m_opengl_text[i]->x();
		text_y = std::max(text_y, m_opengl_text[i]->y());
	}

	double screen_width = 1.0;
	double screen_height = 600./800.;
	double texture_ar = text_x/text_y;
	double texture_width = std::min(1.0, text_x/800.);
	double texture_height = texture_width / texture_ar;

	double svg_ar = m_width/m_height;
	double svg_width = m_width;
	double svg_height = m_height;

	double position_x;
	double position_y;

	if (m_align == CENTER) {
		position_x = -texture_width/2.;
		position_y = -screen_height/2. + (m_y-m_text.fontsize) * screen_height / svg_height;
	} else {
		position_x = -screen_width/2. + (m_x) * screen_width / svg_width;
		position_y = -screen_height/2. + (m_y-m_text.fontsize) * screen_height / svg_height;
	}

	for (unsigned int i = 0; i < _text.size(); i++ ) {
		double syllable_x = m_opengl_text[i]->x();
		double syllable_y = text_y;
		double syllable_width = syllable_x *  texture_width / text_x;
		double syllable_height = texture_height;
		double syllable_ar = syllable_width / syllable_height;
		Dimensions dim(syllable_ar);
		dim.fixedHeight(texture_height).center(position_y + 0.5 * texture_height);
		dim.middle(position_x + 0.5 * dim.w());
		TexCoords tex;
		double factor = _text[i].factor;
		if (factor != 1.0) {
			glColor3f(1.0, 0.8, 0.0);
			dim.fixedWidth(dim.w() * factor);
		}
		if (m_valign == TOP) {
			dim.screenTop();
		}
		m_opengl_text[i]->draw(dim, tex);
		if (factor != 1.0) glColor3f(1.0, 1.0, 1.0);
		position_x += syllable_width;
	}
}

