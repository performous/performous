#include "opengl_text.hh"
#include <pango/pangocairo.h>
#include <math.h>
#include <iostream>
#include <sstream>

namespace {
	unsigned int nextPow2(double _val) {
		unsigned int ret = 1;
		unsigned int val = ceil(_val);
		while (ret < val) ret *= 2;
		return ret;
	}
}

OpenGLText::OpenGLText(TThemeTxtOpenGL &_text) {
	// set some pango values
	PangoAlignment alignment;
	PangoWeight weight;
	PangoStyle style;
	PangoLayout *layout;

	if( _text.fontfamily.empty() )
		_text.fontfamily = std::string("Sans");
	if( _text.fontalign == std::string("start") ) {
		alignment = PANGO_ALIGN_LEFT;
	} else if( _text.fontalign == std::string("center") ) {
		alignment = PANGO_ALIGN_CENTER;
	} else if( _text.fontalign == std::string("end") ) {
		alignment = PANGO_ALIGN_RIGHT;
	} else {
		alignment = PANGO_ALIGN_LEFT;
	}
	if( _text.fontweight == std::string("normal") ) {
		weight = PANGO_WEIGHT_NORMAL;
	} else if( _text.fontweight == std::string("bold") ) {
		weight = PANGO_WEIGHT_BOLD;
	} else if( _text.fontweight == std::string("bolder") ) {
		weight = PANGO_WEIGHT_ULTRABOLD;
	} else {
		weight = PANGO_WEIGHT_NORMAL;
	}
	if( _text.fontstyle == std::string("normal") ) {
		style = PANGO_STYLE_NORMAL;
	} else if( _text.fontstyle == std::string("italic") ) {
		style = PANGO_STYLE_ITALIC;
	} else if( _text.fontstyle == std::string("oblique") ) {
		style = PANGO_STYLE_OBLIQUE;
	} else {
		style = PANGO_STYLE_NORMAL;
	}
	// set font description
	PangoFontDescription *desc = pango_font_description_new();
	pango_font_description_set_weight(desc, weight);
	pango_font_description_set_style(desc, style);
	pango_font_description_set_family(desc, _text.fontfamily.c_str());
	pango_font_description_set_absolute_size (desc,_text.fontsize * PANGO_SCALE);

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
	m_x_power_of_two = nextPow2(m_x);
	m_y_power_of_two = nextPow2(m_y);

	g_object_unref (layout);
	g_object_unref (ctx);

	// create surface
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_x_power_of_two, m_y_power_of_two);
	cairo_t *dc = cairo_create(surface);

	// draw the surface
	layout = pango_cairo_create_layout(dc);
	pango_layout_set_alignment(layout, alignment);
	pango_layout_set_font_description (layout, desc);
	pango_layout_set_text (layout, _text.text.c_str(), -1);
	cairo_save(dc);
	pango_cairo_show_layout (dc, layout);
	pango_cairo_layout_path(dc,layout);
	if (_text.fill_col.r != -1 && _text.fill_col.g != -1 && _text.fill_col.b != -1) {
		cairo_set_source_rgba(dc, _text.fill_col.r, _text.fill_col.g, _text.fill_col.b, _text.fill_col.a);
		if (_text.stroke_col.r != -1 && _text.stroke_col.g != -1 && _text.stroke_col.b != -1) cairo_fill_preserve(dc);
		else cairo_fill(dc);
	}
	if (_text.stroke_col.r != -1 && _text.stroke_col.g != -1 && _text.stroke_col.b != -1) {
		cairo_set_line_width(dc, _text.stroke_width);
		cairo_set_source_rgba(dc, _text.stroke_col.r, _text.stroke_col.g, _text.stroke_col.b, _text.stroke_col.a);
		cairo_stroke(dc);
	}
	pango_cairo_update_layout (dc, layout);
	cairo_restore(dc);
	g_object_unref(layout);

	m_texture.reset(
		new OpenGLTexture(
			cairo_image_surface_get_width(surface),
			cairo_image_surface_get_height(surface),
			OpenGLTexture::INT_ARGB,
			cairo_image_surface_get_data(surface)
		)
	);

	// delete surface
	cairo_destroy(dc);
	cairo_surface_destroy(surface);
	// delete the font description
	pango_font_description_free(desc);
}

void OpenGLText::draw(Dimensions &_dim, TexCoords &_tex) {
	_tex.x2 = _tex.x2 * m_x / m_x_power_of_two;
	_tex.y2 = _tex.y2 * m_y / m_y_power_of_two;
	m_texture->draw(_dim,_tex);
}

OpenGLText::~OpenGLText() {
}

#include <libxml++/libxml++.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
TRGBA getcolor(std::istream& is) {
	std::string tmp;
	std::getline(is, tmp);
	char const* string = tmp.c_str(); // XXX: fix the parsing code to use stream instead
	TRGBA col;
	if (string[0] == '#') {
		if (strlen(string) == 7) {
			unsigned int r,g,b;
			sscanf((string+1), "%02x %02x %02x", &r, &g, &b);
			col.r = (double) r / 255;
			col.g = (double) g / 255;
			col.b = (double) b / 255;
		}
	} else if (!strcasecmp(string, "red")) {
		col.r = 1;
		col.g = col.b = 0;
	} else if (!strcasecmp(string, "lime")) {
		col.g = 1;
		col.r = col.b = 0;
	} else if (!strcasecmp(string, "blue")) {
		col.b = 1;
		col.r = col.g = 0;
	} else if (!strcasecmp(string, "black")) {
		col.r = col.g = col.b = 0;
	} else if (!strcasecmp(string, "silver")) {
		col.r = col.g = col.b = 0.75;
	} else if (!strcasecmp(string, "gray")) {
		col.r = col.g = col.b = 0.5;
	} else if (!strcasecmp(string, "white")) {
		col.r = col.g = col.b = 1;
	} else if (!strcasecmp(string, "maroon")) {
		col.r = 0.5;
		col.g = col.b = 0;
	} else if (!strcasecmp(string, "purple")) {
		col.g = 0.5;
		col.r = col.b = 0.5;
	} else if (!strcasecmp(string, "fuchsia")) {
		col.g = 0.5;
		col.r = col.b = 1;
	} else if (!strcasecmp(string, "green")) {
		col.g = 0.5;
		col.r = col.b = 0;
	} else if (!strcasecmp(string, "olive")) {
		col.b = 0;
		col.r = col.g = 0.5;
	} else if (!strcasecmp(string, "yellow")) {
		col.b = 0;
		col.r = col.g = 1;
	} else if (!strcasecmp(string, "navy")) {
		col.b = 0.5;
		col.r = col.g = 0;
	} else if (!strcasecmp(string, "teal")) {
		col.r = 0;
		col.g = col.b = 0.5;
	} else if (!strcasecmp(string, "aqua")) {
		col.r = 0;
		col.g = col.b = 1;
	} else if (!strcasecmp((string), "none")) {
		col.r = col.g = col.b = -1;
	}
	return col;
}

SvgTxtTheme::SvgTxtTheme(std::string _theme_file, Align _a, VAlign _v, Gravity _g, Fitting _f) : m_gravity(_g), m_fitting(_f), m_valign(_v), m_align(_a) {
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
		else if (token == "fill") m_text.fill_col = getcolor(iss2);
		else if (token == "stroke") m_text.stroke_col = getcolor(iss2);
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

void SvgTxtTheme::draw(std::string _text) {
	if( m_text.text != _text ) {
		m_text.text = _text;
		m_opengl_text.reset(new OpenGLText(m_text));
	}
	
	// 1.0 is the width
	double screen_width = 1.0;
	double screen_height = 600./800.;
	double texture_ar = m_opengl_text->x()/m_opengl_text->y();
	double texture_width = std::min(1.0, m_opengl_text->x()/800.);
	double texture_height = texture_width / texture_ar;
	double svg_ar = m_width/m_height;
	double svg_width = m_width;
	double svg_heigh = m_height;
	double texture_y = -screen_height/2. + (m_y-m_text.fontsize) * screen_height / svg_heigh;
	double texture_x = -screen_width/2. + (m_x) * screen_width / svg_width;
	Dimensions dim = Dimensions(texture_ar).top(texture_y).fixedHeight(texture_height);
	if (m_align == CENTER) dim.middle(); else dim.left(texture_x);

	TexCoords tex;
	tex.x1 = tex.y1 = 0.0f;
	tex.x2 = tex.y2 = 1.0f;
	m_opengl_text->draw(dim,tex);
};

