#include "surface.hh"

// Disabling temporarily: #include <boost/filesystem.hpp>
#include <stdexcept>

Surface::Surface(unsigned int width, unsigned int height, Surface::Format format, unsigned char* buffer) {
	load(width, height, format, buffer);
}

Surface::~Surface() {
	glDeleteTextures(1, &texture_id);
}

namespace {
	bool isPow2(unsigned int val) {
		unsigned int count = 0;
		do { if (val & 1) ++count; } while (val >>= 1);
		return val == 1;
	}

	unsigned int nextPow2(unsigned int val) {
		unsigned int ret = 1;
		while (ret < val) ret *= 2;
		return ret;
	}
}

bool checkExtension(const char *extension)
{
	const GLubyte *extensions = NULL;
	const GLubyte *start;
	GLubyte *where, *terminator;
	
	/* Extension names should not have spaces. */
	where = (GLubyte *) strchr(extension, ' ');
	if (where || *extension == '\0')
		return false;
	extensions = glGetString(GL_EXTENSIONS);
	/* It takes a bit of care to be fool-proof about parsing the
		 OpenGL extensions string. Don't be fooled by sub-strings,
		 etc. */
	start = extensions;
	for (;;) {
		where = (GLubyte *) strstr((const char *) start, extension);
		if (!where)
			break;
		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return true;
		start = terminator;
	}
	return false;
}

void Surface::load(unsigned int width, unsigned int height, Format format, unsigned char* buffer, float ar) {
	m_width = width; m_height = height;
	dimensions = Dimensions(ar != 0.0f ? ar : float(width) / height).fixedWidth(1.0f);
	tex.x1 = tex.y1 = 0.0f;
	tex.x2 = tex.y2 = 1.0f;
	glGenTextures(1, &texture_id);

	bool hasTexture_non_power_of_two = checkExtension("GL_ARB_texture_non_power_of_two");

	if (hasTexture_non_power_of_two) {
		glBindTexture(GL_TEXTURE_2D, texture_id);

		// when texture area is small, bilinear filter the closest mipmap
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
		// when texture area is large, bilinear filter the original
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
	
		// the texture wraps over at the edges (repeat)
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	} else {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texture_id);
	}

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

	if (hasTexture_non_power_of_two) { // Use OpenGL 2.0 functionality 
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, fmt, buffer_fmt, buffer);	
	}
	else {
		//if (isPow2(width) && isPow2(height)) gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, fmt, buffer_fmt, buffer);
		// TODO: Test for OpenGL extension GL_ARB_texture_non_power_of_two and if not found, use gluScaleImage to upscale the texture to nextPow2 dimensions before calling glTexImage2D (if it isn't pow2 already).
	        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, width, height, 0, fmt, buffer_fmt, buffer);	
//		gluScaleImage(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, width, height, 0, fmt, buffer_fmt, buffer);
	}
}

Surface::Use::Use(Surface& s) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s.texture_id);
}

Surface::Use::~Use() {
	glDisable(GL_TEXTURE_2D);
}

void Surface::draw() {
	Use texture(*this);
	glBegin(GL_QUADS);
	float x1 = dimensions.x1();
	float x2 = dimensions.x2();
	float y1 = dimensions.y1();
	float y2 = dimensions.y2();
	glTexCoord2f(tex.x1, tex.y1); glVertex2f(x1, y1);
	glTexCoord2f(tex.x2, tex.y1); glVertex2f(x2, y1);
	glTexCoord2f(tex.x2, tex.y2); glVertex2f(x2, y2);
	glTexCoord2f(tex.x1, tex.y2); glVertex2f(x1, y2);
	glEnd();
}

#include <fstream>

Surface::Surface(std::string filename, Filetype filetype) {
	// Disabling temporarily to get rid of Boost.Filesystem dep: if (!boost::filesystem::is_regular(filename)) throw std::runtime_error("File not found: " + filename);
	if (!std::ifstream(filename.c_str()).is_open()) throw std::runtime_error("File not found: " + filename);
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
			// Find SVG dimensions (in pixels)
			RsvgHandle* svgHandle = rsvg_handle_new_from_file(filename.c_str(), &pError);
			if (pError) {
				g_error_free(pError);
				throw std::runtime_error("Unable to load " + filename);
			}
			RsvgDimensionData svgDimension;
			rsvg_handle_get_dimensions (svgHandle, &svgDimension);
			rsvg_handle_free(svgHandle);
			unsigned int w = nextPow2(svgDimension.width);
			unsigned int h = nextPow2(svgDimension.height);
			// Load and raster the SVG
			GdkPixbuf* pb = rsvg_pixbuf_from_file_at_size(filename.c_str(), w, h, &pError);
			if (pError) {
				g_error_free(pError);
				throw std::runtime_error("Unable to load " + filename);
			}
			load(w, h, CHAR_RGBA, gdk_pixbuf_get_pixels(pb), float(svgDimension.width)/svgDimension.height);
			gdk_pixbuf_unref(pb);
			rsvg_term();
			break;
		}
	}
}

Surface::Surface(cairo_surface_t* _surf) {
	unsigned int w = cairo_image_surface_get_width(_surf);
	unsigned int h = cairo_image_surface_get_height(_surf);
	load(w, h, INT_ARGB, cairo_image_surface_get_data(_surf));
}

