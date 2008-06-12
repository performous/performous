#include "surface.h"

// Disabling temporarily: #include <boost/filesystem.hpp>
#include <stdexcept>

Surface::Surface(unsigned int width, unsigned int height, Surface::Format format, unsigned char* buffer) {
	load(width, height, format, buffer);
}

Surface::~Surface() {
	glDeleteTextures(1, &texture_id);
}

void Surface::load(unsigned int width, unsigned int height, Format format, unsigned char* buffer) {
	m_width = width; m_height = height;
	dimensions = Dimensions(float(width) / height).fixedWidth(1.0f);
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texture_id);
	unsigned int fmt;
	unsigned int buffer_fmt;
	bool swap;
	switch(format) {
	  case RGB:
		fmt = GL_RGB;
		buffer_fmt = GL_UNSIGNED_BYTE;
		swap = false;
		break;
	  case BGR:
		fmt = GL_RGB;
		buffer_fmt = GL_UNSIGNED_BYTE;
		swap = true;
		break;
	  case INT_ARGB:
		buffer_fmt = GL_UNSIGNED_INT_8_8_8_8;
		fmt = GL_BGRA;
		swap = true;
		break;
	  case CHAR_RGBA:
		fmt = GL_RGBA;
		buffer_fmt = GL_UNSIGNED_BYTE;
		swap = false;
		break;
	  default:
		throw std::runtime_error("Unknown pixel format");
	}
	glPixelStorei(GL_UNPACK_SWAP_BYTES, swap );
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, width, height, 0, fmt, buffer_fmt, buffer);
}

void Surface::draw() {
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texture_id);
	glBegin(GL_QUADS);
	float x1 = dimensions.x1();
	float x2 = dimensions.x2();
	float y1 = dimensions.y1();
	float y2 = dimensions.y2();
	glTexCoord2f(0.0f, 0.0f); glVertex2f(x1, y1);
	glTexCoord2f(m_width, 0.0f); glVertex2f(x2, y1);
	glTexCoord2f(m_width, m_height); glVertex2f(x2, y2);
	glTexCoord2f(0.0f, m_height); glVertex2f(x1, y2);
	glEnd();
}

Surface::Surface(std::string filename, Filetype filetype) {
	// Disabling temporarily to get rid of Boost.Filesystem dep: if (!boost::filesystem::is_regular(filename)) throw std::runtime_error("File not found: " + filename);
	switch( filetype ) {
	  case MAGICK: 
	  	{
		Magick::Image image;
		Magick::Blob blob;
		image.read(filename);
		image.magick("RGBA");
		image.write(&blob);
		load(image.columns(), image.rows(), CHAR_RGBA, (unsigned char*)blob.data());
		break;
		}
	  case SVG:
	  	{
		rsvg_init();
		GError* pError = NULL;
		// FIXME: this does not detect errors (file missing/invalid)
		RsvgHandle* svgHandle = rsvg_handle_new_from_file(filename.c_str(), &pError);
		if (pError) {
			g_error_free(pError);
			throw std::runtime_error("Unable to load " + filename);
		}
		RsvgDimensionData svgDimension;
		rsvg_handle_get_dimensions (svgHandle, &svgDimension);
		cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, svgDimension.width, svgDimension.height);
		cairo_t* dc = cairo_create(surface);
		rsvg_handle_render_cairo(svgHandle, dc);
		load(svgDimension.width, svgDimension.height, INT_ARGB, cairo_image_surface_get_data(surface));
		cairo_surface_destroy(surface);
		rsvg_handle_free(svgHandle);
		rsvg_term();
		cairo_destroy(dc);
		break;
		}
	}
}

Surface::Surface(cairo_surface_t* _surf) {
	unsigned int w = cairo_image_surface_get_width(_surf);
	unsigned int h = cairo_image_surface_get_height(_surf);
	load(w, h, INT_ARGB, cairo_image_surface_get_data(_surf));
}

