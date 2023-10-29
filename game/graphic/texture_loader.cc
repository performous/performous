#include "texture_loader.hh"

#include "glutil.hh"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <thread>

namespace {

    class TextureLoader {
    public:
        static TextureLoader& instance() {
            if (!m_instance)
                m_instance = std::make_unique<TextureLoader>();

            return *m_instance;
        }
        static void destroy() {
            m_instance.reset();
        }

        TextureLoader() : m_thread(&TextureLoader::run, this) {}
        ~TextureLoader() {
            m_quit = true;
            m_condition.notify_one();
            m_thread.join();
        }
        /// The loader main loop: poll for image load jobs and load into RAM
        void run() {
            while (!m_quit) {
                fs::path name;
                {
                    // Poll for jobs to be done
                    std::unique_lock<std::mutex> l(m_mutex);
                    for (auto& job : m_jobs) {
                        if (job.done)
                            continue;  // Job already done
                        name = job.name;
                        break;
                    }
                    // If not found, wait for one
                    if (name.empty()) {
                        m_condition.wait(l);
                        continue;
                    }
                }
                // Load image file into buffer
                std::cout << "load bitmap " << name.string() << std::endl;
                Bitmap bitmap;
                load(bitmap, name);
                // Store the result
                std::lock_guard<std::mutex> l(m_mutex);
                for (auto& job : m_jobs) {
                    if (job.name == name) {
                        job.done = true;  // Mark the job completed
                        job.bitmap.swap(bitmap);  // Store the bitmap (if we got any)
                        std::cout << "done loading " << name.string() << std::endl;
                    }
                }
            }
        }
        /// Add a new job, using calling Texture's address as unique ID.
        void push(Job const& job) {
            std::lock_guard<std::mutex> l(m_mutex);
            m_jobs.emplace_back(job);
            m_condition.notify_one();
        }
        /// Cancel a job in progress (no effect if the job has already completed)
    //    void remove(void const* t) {
    //        std::lock_guard<std::mutex> l(m_mutex);
    //        std::cout << "remove " << m_jobs[t].name.string() << std::endl;
    //        m_jobs.erase(t);
    //    }
        /// Upload all completed jobs to OpenGL (must be called from a valid OpenGL context)
        void apply() {
            std::lock_guard<std::mutex> l(m_mutex);
            for (auto it = m_jobs.begin(); it != m_jobs.end();) {
                auto& job = *it;
                if (!job.done) {   // Job incomplete, skip it
                    std::cout << "pending " << job.name.string() << std::endl;
                    ++it;
                    continue;
                }
                std::cout << "apply " << job.name.string() << std::endl;
                job.apply(job.bitmap);  // Upload to OpenGL
                it = m_jobs.erase(it);
            }
        }

    private:
        std::atomic<bool> m_quit{ false };
        std::mutex m_mutex;
        std::condition_variable m_condition;
        using Jobs = std::list<Job>;
        Jobs m_jobs;
        std::thread m_thread;

        static std::unique_ptr<TextureLoader> m_instance;
    };

    std::unique_ptr<TextureLoader> TextureLoader::m_instance;
}

void updateTextures() {
    TextureLoader::instance().apply();
}

void loadAsync(Job const& job) {
    // Ask the loader to retrieve the image
    TextureLoader::instance().push(job);
}

// Stuff for converting pix::Format into OpenGL enum values & other flags
namespace {
    struct PixFmt {
        PixFmt() = default;
        PixFmt(GLenum f, GLenum t, bool s) : format(f), type(t), swap(s) {}
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

void TextureReferenceLoader::load(Bitmap const& bitmap, bool isText) {
    glutil::GLErrorChecker glerror("TextureManager::load");

    m_textureReference->setGeometry(static_cast<float>(bitmap.width), static_cast<float>(bitmap.height), bitmap.ar);
    m_textureReference->setPremultiplied(bitmap.linearPremul);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureReference->getId());

    // When texture area is small, bilinear filter the closest mipmap
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, isText ? GL_LINEAR : GL_LINEAR_MIPMAP_NEAREST);
    // When texture area is large, bilinear filter the original
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, isText ? GL_NEAREST : GL_LINEAR);
    if (!isText)
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    glerror.check("glTexParameterf");

    // Anisotropy is potential trouble maker
    if (epoxy_has_gl_extension("GL_EXT_texture_filter_anisotropic")) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
        glerror.check("MAX_ANISOTROPY_EXT");
    }

    // Load the data into texture
    auto const& f = getPixFmt(bitmap.fmt);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, f.swap);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat(bitmap.linearPremul), bitmap.width, bitmap.height, 0, f.format, f.type, bitmap.data());
    if (!isText)
        glGenerateMipmap(GL_TEXTURE_2D);
}

TextureLoaderScopedKeeper::TextureLoaderScopedKeeper()
{
}

TextureLoaderScopedKeeper::~TextureLoaderScopedKeeper() {
    TextureLoader::destroy();
}
