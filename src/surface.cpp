#include "surface.h"

Surface::Surface(double width, double height, unsigned int format, unsigned char * buffer):
	m_width(width),m_height(height),m_format(format),texture_id(0) {
	glLoad(buffer);
}

Surface::~Surface() {
	glUnload();
}

void Surface::glLoad(unsigned char * buffer) {
	glUnload();
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texture_id);
	unsigned int format;
	bool swap;
	switch(m_format) {
		case SURFACE_RGBA:
			format = GL_RGBA;
			swap = false;
			break;
		case SURFACE_ARGB:
			format = GL_BGRA;
			swap = true;
			break;
	}
	glPixelStorei(GL_UNPACK_SWAP_BYTES, swap );
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, buffer);
}

void Surface::glUnload() {
	if( texture_id ) {
		glDeleteTextures (1, &texture_id);
		texture_id = 0;
	}
}

void Surface::draw( float x, float y, float w, float h) {
	if( texture_id == 0 ) {
		return;
	}
	glMatrixMode(GL_MODELVIEW);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texture_id);
	glPushMatrix();
	glTranslatef(-0.5f + x, -0.5f + y, 0.0f);
	glBegin(GL_QUADS);
	glTexCoord2f(   0.0f,     0.0f); glVertex2f(-0.5f    , -0.5f    );
	glTexCoord2f(m_width,     0.0f); glVertex2f(-0.5f + w, -0.5f    );
	glTexCoord2f(m_width, m_height); glVertex2f(-0.5f + w, -0.5f + h);
	glTexCoord2f(   0.0f, m_height); glVertex2f(-0.5f    , -0.5f + h);
	glEnd();
	glPopMatrix();
}

Surface::Surface( std::string filename, unsigned int type ):texture_id(0) {
	if( type == FILE_SVG ) {
		cairo_t* dc;
		cairo_surface_t* surface;
		RsvgHandle* svgHandle=NULL;
		GError* pError = NULL;
		RsvgDimensionData svgDimension;
		rsvg_init();
		svgHandle = rsvg_handle_new_from_file(filename.c_str(), &pError);
		if(pError != NULL) {
			fprintf (stderr, "CairoSVG::CairoSVG: %s\n", pError->message);
			g_error_free(pError);
		}
		rsvg_handle_get_dimensions (svgHandle, &svgDimension);
		
		m_width  = svgDimension.width;
		m_height = svgDimension.height;
		m_format = SURFACE_ARGB;
		
		surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, m_width, m_height);
		dc = cairo_create(surface);
		rsvg_handle_render_cairo (svgHandle,dc);
		glLoad(cairo_image_surface_get_data(surface));
		rsvg_handle_free (svgHandle);
		rsvg_term();
		cairo_destroy (dc);
	} else if( type == FILE_MAGICK ) {
	}
}
