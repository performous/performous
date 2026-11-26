#include "texture.hh"

#include "configuration.hh"
#include "game.hh"
#include "graphic/video_driver.hh"
#include "log.hh"
#include "screen.hh"
#include "svg.hh"
#include "texturejobqueue.hh"
#include "util.hh"

#include <cctype>
#include <stdexcept>
#include <sstream>
#include <type_traits>
#include <vector>

Shader& getShader(Window& window, std::string const& name) {
	return window.shader(name);  // FIXME
}

float Dimensions::screenY() const {
	switch (m_screenAnchor) {
	  case YAnchor::CENTER: return 0.0f;
	  case YAnchor::TOP: return -0.5f * virtH();
	  case YAnchor::BOTTOM: return 0.5f * virtH();
	}
	throw std::logic_error("Dimensions::screenY(): unknown m_screenAnchor value");
}

namespace {
/// Load a file from disk into a buffer
void load(Bitmap& bitmap, fs::path const& name) {
	try {
		auto const ext = toLower(name.extension().string());
		if (!fs::is_regular_file(name))
		{
			throw std::runtime_error("File not found: " + name.string());
		}
		else
		{
			const ImageType image_type{getImageType(name.string())};
			if (image_type == ImageType::SVG)
				loadSVG(bitmap, name);
			else if (image_type == ImageType::JPEG)
				loadJPEG(bitmap, name);
			else if (image_type == ImageType::PNG)
				loadPNG(bitmap, name);
			else if (image_type == ImageType::WEBP)
				loadWEBP(bitmap, name);
			else
				throw std::runtime_error("Unknown image file format: " + name.string());
		}
	}
	catch (std::exception& e) {
		SpdLogger::error(LogSystem::IMAGE, "Error loading texture, exception={}", e.what());
	}
}

/// The one loader, created and destroyed by TextureLoader. Only used in this file.
std::unique_ptr<TextureJobQueue> gTextureLoader = nullptr;
}  // namespace

static_assert(std::is_same_v<TextureLoader::Ticket, TextureJobQueue::Ticket>, "texture.hh declares Ticket separately to avoid including texturejobqueue.hh");

TextureLoader::TextureLoader() {
	if (gTextureLoader) throw std::logic_error("Texture Loader initialized twice. There can be only one.");
	gTextureLoader = std::make_unique<TextureJobQueue>(load);
}

TextureLoader::~TextureLoader() { gTextureLoader.reset(); }

void updateTextures() { gTextureLoader->apply(); }

template <typename T> TextureLoader::Ticket loader(T* target, fs::path const& name) {
	// Temporarily add 1x1 pixel black texture
	Bitmap bitmap;
	bitmap.fmt = pix::Format::RGB;
	bitmap.resize(1, 1);
	target->load(bitmap);
	// Ask the loader to retrieve the image. The callback only runs on the main thread, and ~Texture()
	// removes the job first, so it never runs for a destroyed Texture.
	return gTextureLoader->push(name, [target](Bitmap& bitmap){ target->load(bitmap); });
}

Texture::Texture(fs::path const& filename) { m_loadTicket = loader(this, filename); }
Texture::~Texture() { if (m_loadTicket) gTextureLoader->remove(m_loadTicket); }

// Stuff for converting pix::Format into OpenGL enum values & other flags
namespace {
	struct PixFmt {
		PixFmt() = default;
		PixFmt(GLenum f, GLenum t, bool s): format(f), type(t), swap(s) {}
		GLenum format = GL_RGB;
		GLenum type = GL_UNSIGNED_BYTE;
		bool swap = false;  // Reverse byte order
	};
	struct PixFormats {
		typedef std::map<pix::Format, PixFmt> Map;
		Map m;
		PixFormats() {
			using namespace pix;
			m[Format::RGB] = PixFmt(GL_RGB, GL_UNSIGNED_BYTE, false);
			m[Format::BGR] = PixFmt(GL_BGR, GL_UNSIGNED_BYTE, true);
			m[Format::CHAR_RGBA] = PixFmt(GL_RGBA, GL_UNSIGNED_BYTE, false);
			m[Format::INT_ARGB] = PixFmt(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, true);
		}
	} pixFormats;
	PixFmt const& getPixFmt(pix::Format format) {
		PixFormats::Map::const_iterator it = pixFormats.m.find(format);
		if (it != pixFormats.m.end()) return it->second;
		throw std::logic_error("Unknown pixel format");
	}
	GLint internalFormat(bool linear) {
		return (!linear && GL_EXT_framebuffer_sRGB ? GL_SRGB_ALPHA : GL_RGBA);
	}
}

void Texture::load(Bitmap const& bitmap, bool isText) {
	glutil::GLErrorChecker glerror("Texture::load");
	// Initialize dimensions
	m_width = static_cast<float>(bitmap.width);
	m_height = static_cast<float>(bitmap.height);
	dimensions = Dimensions(bitmap.ar).fixedWidth(1.0f);
	m_premultiplied = bitmap.linearPremul;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(type(), id());

	// When texture area is small, bilinear filter the closest mipmap
	glTexParameterf(type(), GL_TEXTURE_MIN_FILTER, isText ? GL_LINEAR : GL_LINEAR_MIPMAP_NEAREST);
	// When texture area is large, bilinear filter the original
	glTexParameterf(type(), GL_TEXTURE_MAG_FILTER, isText ? GL_NEAREST : GL_LINEAR);
	if (!isText) glTexParameterf(type(), GL_TEXTURE_MAX_LEVEL, 4);
	glerror.check("glTexParameterf");

	// Anisotropy is potential trouble maker
	if (epoxy_has_gl_extension("GL_EXT_texture_filter_anisotropic")) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
		glerror.check("MAX_ANISOTROPY_EXT");
	}

	// Load the data into texture
	PixFmt const& f = getPixFmt(bitmap.fmt);
	glPixelStorei(GL_UNPACK_SWAP_BYTES, f.swap);
	glTexImage2D(type(), 0, internalFormat(bitmap.linearPremul), bitmap.width, bitmap.height, 0, f.format, f.type, bitmap.data());
	if (!isText) glGenerateMipmap(type());
}

void Texture::draw(Window& window) const {
	if (empty()) return;
	// FIXME: This gets image alpha handling right but our ColorMatrix system always assumes premultiplied alpha
	// (will produce incorrect results for fade effects)
	glBlendFunc(m_premultiplied ? GL_ONE : GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	draw(window, dimensions, TexCoords(tex.x1, tex.y1, tex.x2, tex.y2));
}

void Texture::draw(Window& window, glmath::mat3 const& matrix) const {
	if (empty()) return;
	// FIXME: This gets image alpha handling right but our ColorMatrix system always assumes premultiplied alpha
	// (will produce incorrect results for fade effects)
	glBlendFunc(m_premultiplied ? GL_ONE : GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	draw(window, dimensions, TexCoords(tex.x1, tex.y1, tex.x2, tex.y2), matrix);
}
