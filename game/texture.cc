#include "texture.hh"

#include "configuration.hh"
#include "game.hh"
#include "graphic/video_driver.hh"
#include "log.hh"
#include "screen.hh"
#include "svg.hh"
#include "util.hh"

#include <atomic>
#include <cctype>
#include <condition_variable>
#include <cstdint>
#include <stdexcept>
#include <sstream>
#include <thread>
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
	std::atomic<bool> m_quit{ false };
	std::mutex m_mutex;
	std::condition_variable m_condition;
	using Jobs = std::map<TextureLoader::Ticket, Job>; /// Jobs keyed by ever-increasing ticket; maintains load-ordering
	Jobs m_jobs;
	TextureLoader::Ticket m_lastTicket{ 0 }; 
	std::thread m_thread;
public:
	Impl(): m_quit(), m_thread(&Impl::run, this) {}
	~Impl() {
		{
			std::lock_guard<std::mutex> l(m_mutex);  // Under the lock, so the worker can't miss the wake-up
			m_quit = true;
		}
		m_condition.notify_one();
		m_thread.join();
	}
	/// The loader main loop: poll for image load jobs and load into RAM
	void run() {
		while (!m_quit) {
			TextureLoader::Ticket ticket = 0;
			fs::path name;
			{
				// Take the oldest request that is still pending
				std::unique_lock<std::mutex> l(m_mutex);
				for (auto const& [t, job]: m_jobs) {
					if (job.name.empty()) continue;  // Job already done
					ticket = t;
					name = job.name;
					break;
				}
				// If not found, wait for one
				if (!ticket) {
					if (!m_quit) m_condition.wait(l);  // m_quit is re-checked under the lock to avoid a lost wake-up
					continue;
				}
			}
			// Load image file into buffer
			Bitmap bitmap;
			load(bitmap, name);
			// Store the result
			std::lock_guard<std::mutex> l(m_mutex);
			auto it = m_jobs.find(ticket);
			if (it == m_jobs.end()) continue;  // The job was removed: its Texture was destroyed during the load
			it->second.name.clear();  // Mark the job completed
			it->second.bitmap.swap(bitmap);  // Store the bitmap (if we got any)
		}
	}
	/// Add a new job and return its ticket. A ticket is never reused, so a finished load can only reach the job it was started for.
	TextureLoader::Ticket push(Job job) {
		std::lock_guard<std::mutex> l(m_mutex);
		TextureLoader::Ticket const ticket = ++m_lastTicket;
		m_jobs.emplace(ticket, std::move(job));
		m_condition.notify_one();
		return ticket;
	}
	/// Cancel a job (no effect if it has already been applied)
	void remove(TextureLoader::Ticket ticket) {
		std::lock_guard<std::mutex> l(m_mutex);
		m_jobs.erase(ticket);
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

template <typename T> TextureLoader::Ticket loader(T* target, fs::path const& name) {
	// Temporarily add 1x1 pixel black texture
	Bitmap bitmap;
	bitmap.fmt = pix::Format::RGB;
	bitmap.resize(1, 1);
	target->load(bitmap);
	// Ask the loader to retrieve the image. The callback only runs on the main thread, and ~Texture()
	// removes the job first, so it never runs for a destroyed Texture.
	return ldr->push(Job(name, [target](Bitmap& bitmap){ target->load(bitmap); }));
}

Texture::Texture(fs::path const& filename) { m_loadTicket = loader(this, filename); }
Texture::~Texture() { if (m_loadTicket) ldr->remove(m_loadTicket); }

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
