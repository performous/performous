#include "opengl_text.hh"
#include <pango/pangocairo.h>
#include <math.h>
#include <iostream>

namespace {
	unsigned int nextPow2(double _val) {
		unsigned int ret = 1;
		unsigned int val = ceil(_val);
		while (ret < val) ret *= 2;
		return ret;
	}
}

OpenGLText::OpenGLText(TThemeTxtOpenGL _text) {
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
	PangoRectangle rec;
	PangoContext* ctx=NULL;
	ctx = pango_cairo_font_map_create_context ((PangoCairoFontMap*)pango_cairo_font_map_get_default());
	layout = pango_layout_new(ctx);
	pango_layout_set_alignment(layout, alignment);
	pango_layout_set_font_description (layout, desc);
	pango_layout_set_text (layout, _text.text.c_str(), -1);
	pango_layout_get_pixel_extents (layout,NULL,&rec);
	x = rec.width;
	y = rec.height;
	x_advance = rec.width+rec.x;
	y_advance = rec.height+rec.y;
	x_power_of_two = nextPow2(x);
	y_power_of_two = nextPow2(y);
	g_object_unref (layout);
	g_object_unref (ctx);

	// create surface
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, x_power_of_two, y_power_of_two);
	cairo_t *dc = cairo_create(surface);

	// draw the surface
	layout = pango_cairo_create_layout(dc);
	pango_layout_set_alignment(layout, alignment);
	pango_layout_set_font_description (layout, desc);
	pango_layout_set_text (layout, _text.text.c_str(), -1);
	pango_cairo_update_layout (dc, layout);
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
	_tex.x2 = _tex.x2 * x / x_power_of_two;
	_tex.y2 = _tex.y2 * y / y_power_of_two;
	m_texture->draw(_dim,_tex);
}

OpenGLText::~OpenGLText() {
}
