#include "surface.hh"

#include "video_driver.hh"
#include <Magick++.h>
#include <boost/bind.hpp>
#include <stdexcept>
#include <vector>

// TODO: get rid of this and use C++ std::string instead
#include <string.h>

float Dimensions::screenY() const {
	switch (m_screenAnchor) {
	  case CENTER: return 0.0;
	  case TOP: return -0.5 * virtH();
	  case BOTTOM: return 0.5 * virtH();
	}
	throw std::logic_error("Dimensions::screenY(): unknown m_screenAnchor value");
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

#include <fstream>

template <typename T> void loader(T& target, std::string const& filename) {
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
		target.load(w, h, pix::CHAR_RGBA, gdk_pixbuf_get_pixels(pb), float(svgDimension.width)/svgDimension.height);
		gdk_pixbuf_unref(pb);
		rsvg_term();
	} else {
		Magick::Image image;
		Magick::Blob blob;
		try {
			image.read(filename);
			image.magick("RGBA");
			image.write(&blob);
			target.load(image.columns(),image.rows(), pix::CHAR_RGBA, static_cast<unsigned char const*>(blob.data()));
		}
		catch( Magick::Exception &error_ ) // add error handling
		{
			throw std::runtime_error("Image Error");
		} 
	}
}

Texture::Texture(std::string const& filename) { loader(*this, filename); }
Surface::Surface(std::string const& filename) { loader(*this, filename); }

// Stuff for converting pix::Format into OpenGL enum values
namespace {
	struct PixFmt {
		PixFmt() {} // Required by std::map
		PixFmt(GLenum f, GLenum t, bool s): format(f), type(t), swap(s) {}
		GLenum format;
		GLenum type;
		bool swap;
	};
	struct PixFormats {
		typedef std::map<pix::Format, PixFmt> Map;
		Map m;
		PixFormats() {
			using namespace pix;
			m[RGB] = PixFmt(GL_RGB, GL_UNSIGNED_BYTE, false);
			m[BGR] = PixFmt(GL_RGB, GL_UNSIGNED_BYTE, true);
			m[CHAR_RGBA] = PixFmt(GL_RGBA, GL_UNSIGNED_BYTE, false);
			m[INT_ARGB] = PixFmt(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, true);
		}
	} pixFormats;
	PixFmt const& getPixFmt(pix::Format format) {
		PixFormats::Map::const_iterator it = pixFormats.m.find(format);
		if (it != pixFormats.m.end()) return it->second;
		throw std::logic_error("Unknown pixel format");
	}
}

void Texture::load(unsigned int width, unsigned int height, pix::Format format, unsigned char const* buffer, float ar) {
	m_ar = ar ? ar : double(width) / height;
	UseTexture texture(*this);
	// When texture area is small, bilinear filter the closest mipmap
	glTexParameterf(type(), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	// When texture area is large, bilinear filter the original
	glTexParameterf(type(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(type(), GL_TEXTURE_MAX_LEVEL, 0);
	// The texture wraps over at the edges (repeat)
	glTexParameterf(type(), GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(type(), GL_TEXTURE_WRAP_T, GL_REPEAT);
	PixFmt const& f = getPixFmt(format);
	glPixelStorei(GL_UNPACK_SWAP_BYTES, f.swap);
	// Load the data into texture
	if (checkExtension("GL_ARB_texture_non_power_of_two")) { // Use OpenGL 2.0 functionality 
		glTexImage2D(type(), 0, GL_RGBA, width, height, 0, f.format, f.type, buffer);	
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
		gluScaleImage(f.format, width, height, f.type, buffer, newWidth, newHeight, f.type, &outBuf[0]);
		// BIG FAT WARNING: Do not even think of using ARB_texture_rectangle here!
		// Every developer of the game so far has tried doing so, but it just cannot work.
		// (1) no repeat => cannot texture
		// (2) coordinates not normalized => would require special hackery elsewhere		
		// Just don't do it in Surface class, thanks. -Tronic
		glTexImage2D(type(), 0, GL_RGBA, newWidth, newHeight, 0, f.format, f.type, &outBuf[0]);
	}
}

void Surface::load(unsigned int width, unsigned int height, pix::Format format, unsigned char const* buffer, float ar) {
	using namespace pix;
	// Initialize dimensions
	m_width = width; m_height = height;
	dimensions = Dimensions(ar != 0.0f ? ar : float(width) / height).fixedWidth(1.0f);
	// Load the data into texture
	UseTexture texture(m_texture);
	PixFmt const& f = getPixFmt(format);
	glPixelStorei(GL_UNPACK_SWAP_BYTES, f.swap);
	glTexImage2D(m_texture.type(), 0, GL_RGBA, width, height, 0, f.format, f.type, buffer);	
}

void Surface::draw() const {
	if (m_width * m_height > 0.0) m_texture.draw(dimensions, TexCoords(tex.x1 * m_width, tex.y1 * m_height, tex.x2 * m_width, tex.y2 * m_height));
}

Surface::Surface(cairo_surface_t* _surf) {
	unsigned int w = cairo_image_surface_get_width(_surf);
	unsigned int h = cairo_image_surface_get_height(_surf);
	load(w, h, pix::INT_ARGB, cairo_image_surface_get_data(_surf));
}

