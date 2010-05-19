#include "surface.hh"

#include "fs.hh"
#include "configuration.hh"
#include "video_driver.hh"
#include "image.hh"

#include <fstream>
#include <stdexcept>
#include <sstream>
#include <vector>

#include <cctype>

#include <boost/cstdint.hpp>
using boost::uint32_t;

float Dimensions::screenY() const {
	switch (m_screenAnchor) {
	  case CENTER: return 0.0;
	  case TOP: return -0.5 * virtH();
	  case BOTTOM: return 0.5 * virtH();
	}
	throw std::logic_error("Dimensions::screenY(): unknown m_screenAnchor value");
}


template <typename T> void loader(T& target, fs::path name) {
	std::string const filename = name.string();
	if (!fs::exists(name)) throw std::runtime_error("File not found: " + filename);
	// Get file extension in lower case
	std::string ext = name.extension();
	// somehow this does not convert the extension to lower case:
	//std::for_each(ext.begin(), ext.end(), static_cast<int(*)(int)>(std::tolower));
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower );

	if (ext == ".svg") loadSVG(target, filename);
	else if (ext == ".png") {
		try {loadPNG(target, filename);}
		// FIXME: there is probably a cleaner way to do this
		// FoFiX songs often come with album art that is JPEG with a PNG extension
		catch (...) {loadJPEG(target, filename);}
	}
	else loadJPEG(target, filename);
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
			m[BGR] = PixFmt(GL_BGR, GL_UNSIGNED_BYTE, true);
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
	// The texture wraps over at the edges (repeat)
	glTexParameterf(type(), GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(type(), GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameterf(type(), GL_TEXTURE_MAX_LEVEL, 1);
	glutil::GLErrorChecker glerror1("Texture::load - glTexParameterf");

	// Anisotropy is potential trouble maker
	if (GLEW_EXT_texture_filter_anisotropic)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
	glutil::GLErrorChecker glerror2("Texture::load - MAX_ANISOTROPY_EXT");

	glTexParameteri(type(), GL_GENERATE_MIPMAP, GL_TRUE);
	PixFmt const& f = getPixFmt(format);
	glPixelStorei(GL_UNPACK_SWAP_BYTES, f.swap);
	// Load the data into texture
	if ((isPow2(width) && isPow2(height)) || GLEW_ARB_texture_non_power_of_two) { // Can directly load the texture
		glTexImage2D(type(), 0, GL_RGBA, width, height, 0, f.format, f.type, buffer);
	} else {
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
	// Check for OpenGL errors
	glutil::GLErrorChecker glerror3("Texture::load");
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
	// Check for OpenGL errors
	glutil::GLErrorChecker glerror("Surface::load");
}

void Surface::draw() const {
	if (m_width * m_height > 0.0) m_texture.draw(dimensions, TexCoords(tex.x1 * m_width, tex.y1 * m_height, tex.x2 * m_width, tex.y2 * m_height));
}

Surface::Surface(cairo_surface_t* _surf) {
	unsigned int w = cairo_image_surface_get_width(_surf);
	unsigned int h = cairo_image_surface_get_height(_surf);
	load(w, h, pix::INT_ARGB, cairo_image_surface_get_data(_surf));
}

