#include "surface.h"

#include <boost/filesystem.hpp>
#include <iostream>
#include <stdexcept>

Surface::Surface(double width, double height, Surface::Format format, unsigned char* buffer):
  m_width(width),m_height(height),m_format(format)
{
	glLoad(buffer);
}

Surface::~Surface() {
	std::cout << "~Surf: " << texture_id << std::endl;
	glDeleteTextures(1, &texture_id);
}

void Surface::glLoad(unsigned char* buffer) {
	glGenTextures(1, &texture_id);
	std::cout << "Surf: " << texture_id << std::endl;
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texture_id);
	unsigned int format;
	bool swap;
	switch(m_format) {
		case RGBA:
			format = GL_RGBA;
			swap = false;
			break;
		case ARGB:
			format = GL_BGRA;
			swap = true;
			break;
	}
	glPixelStorei(GL_UNPACK_SWAP_BYTES, swap );
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, buffer);
}

void Surface::draw(float x, float y, float w, float h) {
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

Surface::Surface(std::string filename) {
	if (!boost::filesystem::is_regular(filename)) throw std::runtime_error("File not found: " + filename);
	try {
		Magick::Image image;
		Magick::Blob blob;
		image.read(filename);
		image.magick("RGBA");
		image.write(&blob);
		m_width = image.columns();
		m_height = image.rows();
		m_format = RGBA;
		glLoad((unsigned char*)blob.data());
	} catch (Magick::Exception&) {
		RsvgDimensionData svgDimension;
		rsvg_init();
		GError* pError = NULL;
		// FIXME: this does not detect errors (file missing/invalid)
		RsvgHandle* svgHandle = rsvg_handle_new_from_file(filename.c_str(), &pError);
		if (pError) {
			g_error_free(pError);
			throw std::runtime_error("Unable to load " + filename);
		}
		rsvg_handle_get_dimensions (svgHandle, &svgDimension);

		m_width  = svgDimension.width;
		m_height = svgDimension.height;
		m_format = ARGB;

		cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_width, m_height);
		cairo_t* dc = cairo_create(surface);
		rsvg_handle_render_cairo(svgHandle, dc);
		glLoad(cairo_image_surface_get_data(surface));
		rsvg_handle_free(svgHandle);
		rsvg_term();
		cairo_destroy(dc);
	}
}

