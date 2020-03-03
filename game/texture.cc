#include "texture.hh"

#include "configuration.hh"
#include "video_driver.hh"
#include "screen.hh"
#include "svg.hh"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem.hpp>
#include <atomic>
#include <cctype>
#include <condition_variable>
#include <stdexcept>
#include <sstream>
#include <thread>
#include <vector>

using std::uint32_t;

Shader& getShader(std::string const& name) {
	return Game::getSingletonPtr()->window().shader(name);  // FIXME
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
	typedef std::function<void (Bitmap& bitmap)> ApplyFunc;
	ApplyFunc apply;
	Bitmap bitmap;
	Job() {}
	Job(fs::path const& n, ApplyFunc const& a): name(n), apply(a) {}
};

class TextureLoader::Impl {
	/// Load a file from disk into a buffer
	static void load(Bitmap& bitmap, fs::path const& name) {
		try {
			std::string ext = boost::algorithm::to_lower_copy(name.extension().string());
			if (!fs::is_regular_file(name)) throw std::runtime_error("File not found: " + name.string());
			else if (ext == ".svg") loadSVG(bitmap, name);
			else if (ext == ".jpg" || ext == ".jpeg") loadJPEG(bitmap, name);
			else if (ext == ".png") loadPNG(bitmap, name);
			else throw std::runtime_error("Unknown image file format: " + name.string());
		} catch (std::exception& e) {
			std::clog << "image/error: " << e.what() << std::endl;
		}
	}
	std::atomic<bool> m_quit{ false };
	std::mutex m_mutex;
	std::condition_variable m_condition;
	typedef std::map<void const*, Job> Jobs;
	Jobs m_jobs;
	std::thread m_thread;
public:
	Impl(): m_quit(), m_thread(&Impl::run, this) {}
	~Impl() { m_quit = true; m_condition.notify_one(); m_thread.join(); }
	/// The loader main loop: poll for image load jobs and load into RAM
	void run() {
		while (!m_quit) {
			void const* target = nullptr;
			fs::path name;
			{
				// Poll for jobs to be done
				std::unique_lock<std::mutex> l(m_mutex);
				for (auto& job: m_jobs) {
					if (job.second.name.empty()) continue;  // Job already done
					name = job.second.name;
					target = job.first;
					break;
				}
				// If not found, wait for one
				if (!target) {
					m_condition.wait(l);
					continue;
				}
			}
			// Load image file into buffer
			Bitmap bitmap;
			load(bitmap, name);
			// Store the result
			std::lock_guard<std::mutex> l(m_mutex);
			auto it = m_jobs.find(target);
			if (it == m_jobs.end()) continue;  // The job has been removed already
			it->second.name.clear();  // Mark the job completed
			it->second.bitmap.swap(bitmap);  // Store the bitmap (if we got any)
		}
	}
	/// Add a new job, using calling Texture's address as unique ID.
	void push(void const* t, Job const& job) {
		std::lock_guard<std::mutex> l(m_mutex);
		m_jobs[t] = job;
		m_condition.notify_one();
	}
	/// Cancel a job in progress (no effect if the job has already completed)
	void remove(void const* t) {
		std::lock_guard<std::mutex> l(m_mutex);
		m_jobs.erase(t);
	}
	/// Upload all completed jobs to OpenGL (must be called from a valid OpenGL context)
	void apply() {
		std::lock_guard<std::mutex> l(m_mutex);
		for (auto it = m_jobs.begin(); it != m_jobs.end();) {
			{
				Job& j = it->second;
				if (!j.name.empty()) { ++it; continue; }  // Job incomplete, skip it
				j.apply(j.bitmap);  // Upload to OpenGL
			}
			m_jobs.erase(it++);
		}
	}
};

std::unique_ptr<TextureLoader::Impl> ldr = nullptr;

TextureLoader::TextureLoader() {
	if (ldr) throw std::logic_error("Texture Loader initialized twice. There can be only one.");
	ldr = std::make_unique<Impl>();
}

TextureLoader::~TextureLoader() { ldr.reset(); }

void updateTextures() { ldr->apply(); }

template <typename T> void loader(T* target, fs::path const& name) {
	// Temporarily add 1x1 pixel black texture
	Bitmap bitmap;
	bitmap.fmt = pix::RGB;
	bitmap.resize(1, 1);
	target->load(bitmap);
	// Ask the loader to retrieve the image
	ldr->push(target, Job(name, [target, filename = name.string()](Bitmap& bitmap)
	{
	if (bitmap.data())
		target->load(bitmap);
	else //TODO: probably we could handle invalid image right after loading, but now it's most conveniently  place :)
		std::clog << "texture/error: can't read data from " << filename << std::endl;
	}));
}

Texture::Texture(fs::path const& filename) { loader(this, filename); }
Texture::~Texture() { ldr->remove(this); }

// Stuff for converting pix::Format into OpenGL enum values & other flags
namespace {
	struct PixFmt {
		PixFmt(): swap() {} // Required by std::map
		PixFmt(GLenum f, GLenum t, bool s): format(f), type(t), swap(s) {}
		GLenum format;
		GLenum type;
		bool swap;  // Reverse byte order
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
	GLint internalFormat(bool linear) {
		return (!linear && GL_EXT_framebuffer_sRGB ? GL_SRGB_ALPHA : GL_RGBA);
	}
}

void Texture::load(Bitmap const& bitmap, bool isText) {
	glutil::GLErrorChecker glerror("Texture::load");
	// Initialize dimensions
	m_width = bitmap.width; m_height = bitmap.height;
	dimensions = Dimensions(bitmap.ar).fixedWidth(1.0f);
	m_premultiplied = bitmap.linearPremul;
	UseTexture texture(*this);
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

void Texture::draw() const {
	if (empty()) return;
	// FIXME: This gets image alpha handling right but our ColorMatrix system always assumes premultiplied alpha
	// (will produce incorrect results for fade effects)
	glBlendFunc(m_premultiplied ? GL_ONE : GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	draw(dimensions, TexCoords(tex.x1, tex.y1, tex.x2, tex.y2));
}

