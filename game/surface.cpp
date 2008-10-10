#include "surface.hh"
#include <vector>
// Disabling temporarily: #include <boost/filesystem.hpp>
#include <stdexcept>
#include <Magick++.h>

// TODO: get rid of this and use C++ std::string instead
#include <string.h>

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

	unsigned int prevPow2(unsigned int val) {
		unsigned int ret = 1;
		while ((ret*2) < val) ret *= 2;
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

OpenGLTexture::OpenGLTexture(unsigned int width, unsigned int height, Format format, unsigned char* buffer) {
	glGenTextures(1, &m_texture_id);

	bool hasTexture_non_power_of_two = checkExtension("GL_ARB_texture_non_power_of_two");

	glBindTexture(GL_TEXTURE_2D, m_texture_id);

	// when texture area is small, bilinear filter the closest mipmap
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
	// when texture area is large, bilinear filter the original
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
	
	// the texture wraps over at the edges (repeat)
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

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
	} else {
		// TODO: Test for OpenGL extension GL_ARB_texture_non_power_of_two and if not found, 
		// use gluScaleImage to upscale the texture to nextPow2 dimensions before calling 
		// glTexImage2D (if it isn't pow2 already).
		// speeds it up... trust me you need it for now at least! :P
		// TODO: remove when cairo is fixed.
		//
		//if (isPow2(width) && isPow2(height))
		//  gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, fmt, buffer_fmt, buffer);
		//	int newWidth = nextPow2(width);
		//	int newHeight = nextPow2(height);
		int newWidth = prevPow2(width);
		int newHeight = prevPow2(height);
		std::vector<uint32_t> outBuf(newWidth * newHeight);
		gluScaleImage(fmt, width, height, buffer_fmt, buffer, newWidth, newHeight, buffer_fmt, &outBuf[0]);
		// BIG FAT WARNING: Do not even think of using ARB_texture_rectangle here!
		// Every developer of the game so far has tried doing so, but it just cannot work.
		// (1) no repeat => cannot texture
		// (2) coordinates not normalized => would require special hackery elsewhere		
		// Just don't do it in Surface class, thanks. -Tronic
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, newWidth, newHeight, 0, fmt, buffer_fmt, &outBuf[0]);
	}
}

Use::Use(OpenGLTexture const& s) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s.id());
}

Use::Use(Surface const& s) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s.texture().id());
}

Use::~Use() {
	glDisable(GL_TEXTURE_2D);
}


void OpenGLTexture::draw(Dimensions &dim, TexCoords &tex) {
	Use texture(*this);
	glBegin(GL_QUADS);
	glTexCoord2f(tex.x1, tex.y1); glVertex2f(dim.x1(), dim.y1());
	glTexCoord2f(tex.x2, tex.y1); glVertex2f(dim.x2(), dim.y1());
	glTexCoord2f(tex.x2, tex.y2); glVertex2f(dim.x2(), dim.y2());
	glTexCoord2f(tex.x1, tex.y2); glVertex2f(dim.x1(), dim.y2());
	glEnd();
}

OpenGLTexture::~OpenGLTexture() {
	glDeleteTextures(1, &m_texture_id);
}

Surface::Surface(unsigned int width, unsigned int height, OpenGLTexture::Format format, unsigned char* buffer) {
	load(width, height, format, buffer);
}

Surface::~Surface() { }

void Surface::load(unsigned int width, unsigned int height, OpenGLTexture::Format format, unsigned char* buffer, float ar) {
	m_texture.reset(new OpenGLTexture(width,height,format,buffer));
	m_width = width; m_height = height;
	dimensions = Dimensions(ar != 0.0f ? ar : float(width) / height).fixedWidth(1.0f);
	tex.x1 = tex.y1 = 0.0f;
	tex.x2 = tex.y2 = 1.0f;
}

void Surface::draw() {
	m_texture->draw(dimensions,tex);
}

#include <fstream>

Surface::Surface(std::string const& filename) {
	// Disabling temporarily to get rid of Boost.Filesystem dep: if (!boost::filesystem::is_regular(filename)) throw std::runtime_error("File not found: " + filename);
	if (!std::ifstream(filename.c_str()).is_open()) throw std::runtime_error("File not found: " + filename);
	if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".svg") {
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
		load(w, h, OpenGLTexture::CHAR_RGBA, gdk_pixbuf_get_pixels(pb), float(svgDimension.width)/svgDimension.height);
		gdk_pixbuf_unref(pb);
		rsvg_term();
	} else {
		Magick::Image image;
		Magick::Blob blob;
		try {
			image.read(filename);
			image.magick("RGBA");
			image.write(&blob);
			load(image.columns(),image.rows(), OpenGLTexture::CHAR_RGBA, (unsigned char*)blob.data());
		}
		catch( Magick::Exception &error_ ) // add error handling
		{
			throw std::runtime_error("Image Error");
		} 
	}
}

Surface::Surface(cairo_surface_t* _surf) {
	unsigned int w = cairo_image_surface_get_width(_surf);
	unsigned int h = cairo_image_surface_get_height(_surf);
	load(w, h, OpenGLTexture::INT_ARGB, cairo_image_surface_get_data(_surf));
}

