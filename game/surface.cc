#include "surface.hh"

#include "filemagic.hh"
#include "fs.hh"
#include "configuration.hh"
#include "video_driver.hh"
#include "image.hh"
#include "screen.hh"

#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <vector>

#include <cctype>

#include <boost/cstdint.hpp>
#include <boost/format.hpp>
using boost::uint32_t;

Shader& getShader(std::string const& name) {
	return ScreenManager::getSingletonPtr()->window().shader(name);  // FIXME
}

float Dimensions::screenY() const {
	switch (m_screenAnchor) {
	  case CENTER: return 0.0;
	  case TOP: return -0.5 * virtH();
	  case BOTTOM: return 0.5 * virtH();
	}
	throw std::logic_error("Dimensions::screenY(): unknown m_screenAnchor value");
}

struct Job {
	fs::path name;
	typedef boost::function<void (Bitmap& bitmap)> ApplyFunc;
	ApplyFunc apply;
	Bitmap bitmap;
	Job() {}
	Job(fs::path const& n, ApplyFunc const& a): name(n), apply(a) {}
};

struct Loader {
	volatile bool m_quit;
	boost::thread m_thread;
	boost::mutex m_mutex;
	boost::condition m_condition;
	typedef std::map<void const*, Job> Jobs;
	Jobs m_jobs;
	Loader(): m_quit(), m_thread(&Loader::run, this) {}
	~Loader() { m_quit = true; m_condition.notify_one(); m_thread.join(); }
	void run() {
		while (!m_quit) {
			void const* target = NULL;
			fs::path name;
			{
				// Poll for jobs to be done
				boost::mutex::scoped_lock l(m_mutex);
				for (Jobs::iterator it = m_jobs.begin(); it != m_jobs.end(); ++it) {
					if (it->second.name.empty()) continue;  // Job already done
					name = it->second.name;
					target = it->first;
				}
				// If not found, wait for one
				if (!target) {
					m_condition.wait(l);
					continue;
				}
			}
			Bitmap bitmap;
			try {
				// Load bitmap from disk
				std::string const filename = name.string();
				if (!fs::exists(name) || fs::is_directory(name)) throw std::runtime_error("File not found: " + filename);
				else if (filemagic::SVG(name)) loadSVG(bitmap, filename);
				else if (filemagic::JPEG(name)) loadJPEG(bitmap, filename);
				else if (filemagic::PNG(name)) loadPNG(bitmap, filename);
				else throw std::runtime_error("Unable to load the image: " + filename);
			} catch (std::exception& e) {
				std::clog << "image/error: " << e.what() << std::endl;
			}
			// Store the result
			boost::mutex::scoped_lock l(m_mutex);
			Jobs::iterator it = m_jobs.find(target);
			if (it == m_jobs.end()) continue;  // The job has been removed already
			it->second.name.clear();  // Mark the job completed
			it->second.bitmap.swap(bitmap);  // Store the bitmap (if we got any)
		}
	}
	void push(void const* t, Job const& job) {
		boost::mutex::scoped_lock l(m_mutex);
		m_jobs[t] = job;
		m_condition.notify_one();
	}
	void remove(void const* t) {
		boost::mutex::scoped_lock l(m_mutex);
		m_jobs.erase(t);
	}
	void apply() {
		boost::mutex::scoped_lock l(m_mutex);
		for (Jobs::iterator it = m_jobs.begin(); it != m_jobs.end();) {
			{
				Job& j = it->second;
				if (!j.name.empty()) { ++it; continue; }  // Job incomplete, skip it
				j.apply(j.bitmap);  // Load to OpenGL
			}
			m_jobs.erase(it++);
		}
	}
} ldr;

void updateSurfaces() { ldr.apply(); }

template <typename T> void loader(T* target, fs::path const& name) {
	// Temporarily add 1x1 pixel gray block
	Bitmap bitmap;
	bitmap.resize(1, 1);
	target->load(bitmap);
	// Ask the loader to retrieve the image
	ldr.push(target, Job(name, boost::bind(&T::load, target, _1)));
}

Texture::Texture(std::string const& filename) { loader(this, filename); }
Surface::Surface(std::string const& filename) { loader(this, filename); }
Texture::~Texture() { ldr.remove(this); }
Surface::~Surface() { ldr.remove(this); }

// Stuff for converting pix::Format into OpenGL enum values
namespace {
	struct PixFmt {
		PixFmt(): swap() {} // Required by std::map
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
	GLint internalFormat() { return GL_EXT_framebuffer_sRGB ? GL_SRGB_ALPHA : GL_RGBA; }
}

void Texture::load(Bitmap const& bitmap) {
	glutil::GLErrorChecker glerror("Texture::load");
	m_ar = bitmap.ar;
	UseTexture texture(*this);
	// When texture area is small, bilinear filter the closest mipmap
	glTexParameterf(type(), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	// When texture area is large, bilinear filter the original
	glTexParameterf(type(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// The texture wraps over at the edges (repeat)
	glTexParameterf(type(), GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(type(), GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(type(), GL_TEXTURE_MAX_LEVEL, GLEW_VERSION_3_0 ? 4 : 0);  // Mipmaps currently b0rked on Intel, so disable them...
	glerror.check("glTexParameterf");

	// Anisotropy is potential trouble maker
	if (GLEW_EXT_texture_filter_anisotropic) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
	glerror.check("MAX_ANISOTROPY_EXT");

	PixFmt const& f = getPixFmt(bitmap.fmt);
	glPixelStorei(GL_UNPACK_SWAP_BYTES, f.swap);
	// Load the data into texture
	glTexImage2D(type(), 0, internalFormat(), bitmap.width, bitmap.height, 0, f.format, f.type, &bitmap.buf[0]);
	glGenerateMipmap(type());
}

void Surface::load(Bitmap const& bitmap) {
	glutil::GLErrorChecker glerror("Surface::load");
	// Initialize dimensions
	m_width = bitmap.width; m_height = bitmap.height;
	dimensions = Dimensions(bitmap.ar).fixedWidth(1.0f);
	// Load the data into texture
	UseTexture texture(m_texture);
	PixFmt const& f = getPixFmt(bitmap.fmt);
	glPixelStorei(GL_UNPACK_SWAP_BYTES, f.swap);
	glTexImage2D(m_texture.type(), 0, internalFormat(), bitmap.width, bitmap.height, 0, f.format, f.type, &bitmap.buf[0]);
}

void Surface::draw() const {
	if (m_width * m_height > 0.0) m_texture.draw(dimensions, TexCoords(tex.x1 * m_width, tex.y1 * m_height, tex.x2 * m_width, tex.y2 * m_height));
}

