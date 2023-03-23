#include "window.hh"

#include "color_trans.hh"
#include "configuration.hh"
#include "game.hh"
#include "platform.hh"
#include "view_trans.hh"
#include "video_driver.hh"

#include <SDL.h>
#include <SDL_hints.h>
#include <SDL_rect.h>
#include <SDL_video.h>

#define stringify( name ) #name

GLuint Window::m_ubo = 0;
GLuint Window::m_vao = 0;
GLuint Window::m_vbo = 0;
GLint Window::bufferOffsetAlignment = -1;

namespace {
	float s_width;
	float s_height;

	/// Attempt to set attribute and report errors.
	/// Tests for success when destoryed.
	struct GLattrSetter {
		GLattrSetter(SDL_GLattr attr, int value): m_attr(attr), m_value(value) {
			if (SDL_GL_SetAttribute(attr, value)) std::clog << "video/warning: Error setting GLattr " << stringify(m_attr) << std::endl;
		}
		~GLattrSetter() {
			int value;
			SDL_GL_GetAttribute(m_attr, &value);
			if (value != m_value)
				std::clog << "video/warning: Error setting GLattr " << stringify (m_attr)
				<< ": requested " << m_value << ", got " << value << std::endl;
		}
		SDL_GLattr m_attr;
		int m_value;
	};
}

float screenW() { return s_width; }
float screenH() { return s_height; }

Window::Window()
: screen(nullptr, &SDL_DestroyWindow), glContext(nullptr, &SDL_GL_DeleteContext) {
	m_system = std::make_unique<SDLSystem>();

	SDL_JoystickEventState(SDL_ENABLE);
	{ // Setup GL attributes for context creation
		SDL_SetHintWithPriority("SDL_HINT_VIDEO_HIGHDPI_DISABLED", "0", SDL_HINT_DEFAULT);
		GLattrSetter attr_hw(SDL_GL_ACCELERATED_VISUAL, 1);
		GLattrSetter attr_flush(SDL_GL_CONTEXT_RELEASE_BEHAVIOR, SDL_GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH);
		GLattrSetter attr_glmaj(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		GLattrSetter attr_glmin(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		GLattrSetter attr_glprof(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		GLattrSetter attr_r(SDL_GL_RED_SIZE, 8);
		GLattrSetter attr_g(SDL_GL_GREEN_SIZE, 8);
		GLattrSetter attr_b(SDL_GL_BLUE_SIZE, 8);
		GLattrSetter attr_a(SDL_GL_ALPHA_SIZE, 8);
		GLattrSetter attr_buf(SDL_GL_BUFFER_SIZE, 32);
		GLattrSetter attr_d(SDL_GL_DEPTH_SIZE, 24);
		GLattrSetter attr_db(SDL_GL_DOUBLEBUFFER, 1);
		Uint32 flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
		if (config["graphic/highdpi"].b()) {
			flags |= SDL_WINDOW_ALLOW_HIGHDPI;
		}
		else {
			SDL_SetHintWithPriority("SDL_HINT_VIDEO_HIGHDPI_DISABLED", "1", SDL_HINT_OVERRIDE);
		}
		int width = config["graphic/window_width"].i();
		int height = config["graphic/window_height"].i();
		int windowPosX = config["graphic/window_pos_x"].i();
		int windowPosY = config["graphic/window_pos_y"].i();
		int displayCount = SDL_GetNumVideoDisplays();
		if (displayCount <= 0) {
			throw std::runtime_error(std::string("video/error: SDL_GetNumVideoDisplays failed: ") + SDL_GetError());
		}
		SDL_Rect totalSize;
		if (displayCount > 1) {
			totalSize.x = 0;
			totalSize.y = 0;
			totalSize.w = 0;
			totalSize.h = 0;
			int displayNum = 0;
			SDL_Rect displaySize;
			SDL_Rect prevTotal;
			while (displayNum < displayCount) {
				if (SDL_GetDisplayBounds(displayNum, &displaySize) != 0) {
					throw std::runtime_error(std::string("video/error: SDL_GetDisplayBounds failed: ") + SDL_GetError());
				}
				prevTotal.x = totalSize.x;
				prevTotal.y = totalSize.y;
				prevTotal.w = totalSize.w;
				prevTotal.h = totalSize.h;
				SDL_UnionRect(&prevTotal, &displaySize, &totalSize);
				++displayNum;
			}
		}
		else {
			if (SDL_GetDisplayBounds(0, &totalSize) != 0) {
				throw std::runtime_error(std::string("video/error: SDL_GetDisplayBounds failed: ") + SDL_GetError());
			}
		}
		SDL_Point winOrigin {windowPosX, windowPosY};
		if (SDL_PointInRect(&winOrigin, &totalSize) == SDL_FALSE) {
			if (winOrigin.x < totalSize.x) {
				winOrigin.x = totalSize.x;
			}
			else if (winOrigin.x > totalSize.w) {
				winOrigin.x = (totalSize.w - width);
			}
			if (winOrigin.y < totalSize.y) {
				winOrigin.y = totalSize.y;
			}
			else if (winOrigin.y > totalSize.h) {
				winOrigin.y = (totalSize.h - height);
			}
			std::clog << "video/info: Saved window position outside of current display set-up; resetting to " << winOrigin.x << "," << winOrigin.y << std::endl;
		}
		SDL_Point winEnd {(winOrigin.x + width), (winOrigin.y + height)};
		if (SDL_PointInRect(&winEnd, &totalSize) == SDL_FALSE) {
			if (winEnd.x > totalSize.w) {
				width = totalSize.w;
				winOrigin.x = (totalSize.w - width);
			}
			if (winEnd.y > totalSize.h) {
				height = totalSize.h;
				winOrigin.y = (totalSize.y - height);
			}
			std::clog << "video/info: Saved window size outside of current display set-up; resetting to " << width << "x" << height << std::endl;
		}

		if (winOrigin.x == 0) {
			winOrigin.x = SDL_WINDOWPOS_UNDEFINED;
		}
		if (winOrigin.y == 0) {
			winOrigin.y = SDL_WINDOWPOS_UNDEFINED;
		}

		std::clog << "video/info: Create window dimensions: " << width << "x" << height << " on screen position: " << winOrigin.x << "x" << winOrigin.y << std::endl;
		screen.reset(SDL_CreateWindow(PACKAGE " " VERSION, winOrigin.x, winOrigin.y, width, height, flags));
		if (!screen) {
			throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
		}
		m_shaderManager = std::make_unique<ShaderManager>();
		glContext.reset(SDL_GL_CreateContext(screen.get()));
		if (glContext == nullptr) throw std::runtime_error(std::string("SDL_GL_CreateContext failed with error: ") + SDL_GetError());
		if (epoxy_gl_version() < 33) throw std::runtime_error("Performous needs at least OpenGL 3.3+ Core profile to run.");
		glutil::GLErrorChecker error("Initializing buffers");
		{
			initBuffers();
		}
	}
	SDL_SetWindowMinimumSize(screen.get(), 640, 360);
	SDL_GetWindowPosition(screen.get(), &m_windowX, &m_windowY);
	// Dump some OpenGL info
	std::clog << "video/info: GL_VENDOR:     " << glGetString(GL_VENDOR) << std::endl;
	std::clog << "video/info: GL_VERSION:    " << glGetString(GL_VERSION) << std::endl;
	std::clog << "video/info: GL_RENDERER:   " << glGetString(GL_RENDERER) << std::endl;
	// Extensions would need more complex outputting, otherwise they will break clog.
	//std::clog << "video/info: GL_EXTENSIONS: " << glGetString(GL_EXTENSIONS) << std::endl;
	createShaders();
	resize();
	SDL_ShowWindow(screen.get());
}

Window::~Window() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindVertexArray(0);
	glDeleteBuffers(1, &m_vbo);
	glDeleteBuffers(1, &m_ubo);
	glDeleteVertexArrays(1, &m_vao);
}

void Window::createShaders() {
	// The Stereo3D shader needs OpenGL 3.3 and GL_ARB_viewport_array, some Intel drivers support GL 3.3,
	// but not GL_ARB_viewport_array, so we just check for the extension here.
	if (config["graphic/stereo3d"].b()) {
		if (epoxy_has_gl_extension("GL_ARB_viewport_array")) {
			// Compile geometry shaders when stereo is requested
			shader("color").compileFile(findFile("shaders/stereo3d.geom"));
			shader("texture").compileFile(findFile("shaders/stereo3d.geom"));
			shader("3dobject").compileFile(findFile("shaders/stereo3d.geom"));
			shader("dancenote").compileFile(findFile("shaders/stereo3d.geom"));
		}
		else {
			std::clog << "video/warning: Stereo3D was enabled but the 'GL_ARB_viewport_array' extension is unsupported; will now disable Stereo3D." << std::endl;
			config["graphic/stereo3d"].b() = false;
		}
	}

	shader("color")
	  .addDefines("#define ENABLE_VERTEX_COLOR\n")
	  .compileFile(findFile("shaders/core.vert"))
	  .compileFile(findFile("shaders/core.frag"))
	  .link()
	  .bindUniformBlocks();
	shader("texture")
	  .addDefines("#define ENABLE_TEXTURING\n")
	  .addDefines("#define ENABLE_VERTEX_COLOR\n")
	  .compileFile(findFile("shaders/core.vert"))
	  .compileFile(findFile("shaders/core.frag"))
	  .link()
	  .bindUniformBlocks();
	shader("3dobject")
	  .addDefines("#define ENABLE_LIGHTING\n")
	  .compileFile(findFile("shaders/core.vert"))
	  .compileFile(findFile("shaders/core.frag"))
	  .link()
	  .bindUniformBlocks();
	shader("dancenote")
	  .addDefines("#define ENABLE_TEXTURING\n")
	  .addDefines("#define ENABLE_VERTEX_COLOR\n")
	  .compileFile(findFile("shaders/dancenote.vert"))
	  .compileFile(findFile("shaders/core.frag"))
	  .link()
	  .bindUniformBlocks();

	updateColor();
	view(0);  // For loading screens
}

Shader& Window::shader(std::string const& name) {
	return m_shaderManager->createOrGet(name);
}

void Window::resetShaders() {
	m_shaderManager->resetShaders();
	createShaders();
}

void Window::initBuffers() {
	glGenVertexArrays(1, &Window::m_vao); // Create VAO.
	glBindVertexArray(Window::m_vao);
	glGenBuffers(1, &Window::m_vbo); // Create VBO.
	glGenBuffers(1, &Window::m_ubo); // Create UBO.

	GLsizei stride = glutil::VertexArray::stride();
	glBindBuffer(GL_ARRAY_BUFFER, Window::m_vbo);

	glEnableVertexAttribArray(vertPos);
	glVertexAttribPointer(vertPos, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(glutil::VertexInfo, vertPos));
	glEnableVertexAttribArray(vertTexCoord);
	glVertexAttribPointer(vertTexCoord, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(glutil::VertexInfo, vertTexCoord));
	glEnableVertexAttribArray(vertNormal);
	glVertexAttribPointer(vertNormal, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(glutil::VertexInfo, vertNormal));
	glEnableVertexAttribArray(vertColor);
	glVertexAttribPointer(vertColor, 4, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(glutil::VertexInfo, vertColor));
}

void Window::blank() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::updateStereo(float sepFactor) {
	try {
		m_stereoUniforms.sepFactor = sepFactor;
		m_stereoUniforms.z0 = (Constant::z0 - 2.0f * Constant::nearDistance);
		glBufferSubData(GL_UNIFORM_BUFFER, m_stereoUniforms.offset(), m_stereoUniforms.size(), &m_stereoUniforms);
	} catch(...) {}  // Not fatal if 3d shader is missing
}

void Window::updateColor() {
	m_matrixUniforms.colorMatrix = Global::color;
	glBufferSubData(GL_UNIFORM_BUFFER, (glutil::shaderMatrices::offset() + static_cast<GLint>(offsetof(glutil::shaderMatrices, colorMatrix))), sizeof(glmath::mat4), &m_matrixUniforms.colorMatrix);
}

void Window::updateLyricHighlight(glmath::vec4 const& fill, glmath::vec4 const& stroke, glmath::vec4 const& newFill, glmath::vec4 const& newStroke) {
	m_lyricColorUniforms.origFill = fill;
	m_lyricColorUniforms.origStroke = stroke;
	m_lyricColorUniforms.newFill = newFill;
	m_lyricColorUniforms.newStroke = newStroke;
	glBufferSubData(GL_UNIFORM_BUFFER, m_lyricColorUniforms.offset(), m_lyricColorUniforms.size(), &m_lyricColorUniforms);
}

void Window::updateLyricHighlight(glmath::vec4 const& fill, glmath::vec4 const& stroke) {
	m_lyricColorUniforms.newFill = fill;
	m_lyricColorUniforms.newStroke = stroke;
	glBufferSubData(GL_UNIFORM_BUFFER, m_lyricColorUniforms.offset(), m_lyricColorUniforms.size(), &m_lyricColorUniforms);
}

void Window::updateTransforms() {
	using namespace glmath;
	mat4 normal(Global::modelview);
	m_matrixUniforms.projMatrix = Global::projection;
	m_matrixUniforms.mvMatrix = Global::modelview;
	m_matrixUniforms.normalMatrix = normal;
	glBufferSubData(GL_UNIFORM_BUFFER, m_matrixUniforms.offset(), m_matrixUniforms.size(), &m_matrixUniforms);
}

void Window::render(Game &game, std::function<void (void)> drawFunc) {
	glutil::GLErrorChecker glerror("Window::render");
	auto& window = game.getWindow();
	ViewTrans trans(*this);  // Default frustum
	bool stereo = config["graphic/stereo3d"].b();
	auto const type = static_cast<Stereo3dType>(config["graphic/stereo3dtype"].ui());

	static bool warn3d = false;
	if (!stereo) warn3d = false;
	if (stereo && !epoxy_has_gl_extension("GL_ARB_viewport_array")) {
		stereo = false;
		if (!warn3d) {
			warn3d = true;
			std::clog << "video/warning: Your GPU does not support Stereo3D mode (OpenGL extension ARB_viewport_array is required)" << std::endl;
			game.flashMessage("Your GPU does not support Stereo3D mode");
		}
	}

	// Over/under only available in fullscreen
	if (stereo && type == Stereo3dType::OverUnder && !m_fullscreen) stereo = false;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	updateStereo(stereo ? getSeparation() : 0.0f);
	glerror.check("setup");
	// Can we do direct to framebuffer rendering (no FBO)?
	if (!stereo || type == Stereo3dType::OverUnder) {
		view(stereo);
		drawFunc();
		return;
	}
	// Render both eyes to FBO (full resolution top/bottom for anaglyph)

	glerror.check("FBO");
	{
		UseFBO user(getFBO());
		blank();
		view(0);
		glViewportIndexedf(1, 0, (getFBO().height() / 2), getFBO().width(), (getFBO().height() / 2));
		glViewportIndexedf(2, 0, 0, getFBO().width(), (getFBO().height() / 2));
		drawFunc();
	}
	glerror.check("Render to FBO");
	// Render to actual framebuffer from FBOs
	UseTexture use(window, getFBO().getTexture());
	view(0);  // Viewport for drawable area
	glDisable(GL_BLEND);
	glmath::mat4 colorMatrix = glmath::mat4(1.0f);
	updateStereo(0.0f);  // Disable stereo mode while we composite
	glerror.check("FBO->FB setup");
	for (int num = 0; num < 2; ++num) {
		{
			float saturation = 0.5f;  // (0..1)
			float col = (1.0f + 2.0f * saturation) / 3.0f;
			float gry = 0.5f * (1.0f - col);
			bool out[3] = {};  // Which colors to output
			if (type == Stereo3dType::RedCyan && num == 0) { out[0] = true; }  // Red
			if (type == Stereo3dType::RedCyan && num == 1) { out[1] = out[2] = true; }  // Cyan
			if (type == Stereo3dType::GreenMagenta && num == 0) { out[1] = true; }  // Green
			if (type == Stereo3dType::GreenMagenta && num == 1) { out[0] = out[2] = true; }  // Magenta
			for (int i = 0; i < 3; ++i) {
				for (int j = 0; j < 3; ++j) {
					float val = 0.0f;
					if (out[i]) val = (i == j ? col : gry);
					colorMatrix[j][i] = val;
				}
			}
		}
		auto& window = game.getWindow();
		// Render FBO with 1:1 pixels, properly filtered/positioned for 3d
		ColorTrans c(window, colorMatrix);
		Dimensions dim = Dimensions(getFBO().width() / getFBO().height()).fixedWidth(1.0f);
		dim.center((num == 0 ? 0.25f : -0.25f) * dim.h());
		if (num == 1) {
			// Right eye blends over the left eye
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
		}
		getFBO().getTexture().draw(window, dim, TexCoords(0.0f, 1.0f, 1.0f, 0));
	}
}

void Window::view(unsigned num) {
	glutil::GLErrorChecker glerror("Window::view");
	// Set flags
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	if (GL_EXT_framebuffer_sRGB) glEnable(GL_FRAMEBUFFER_SRGB);
	glerror.check("setup");
	shader("color").bind();
	// Setup views (with black bars for cropping)
	int nativeW;
	int nativeH;
	if (std::stoi(SDL_GetHint("SDL_HINT_VIDEO_HIGHDPI_DISABLED")) == 1) {
		SDL_GetWindowSize(screen.get(), &nativeW, &nativeH);
	}
	else { SDL_GL_GetDrawableSize(screen.get(), &nativeW, &nativeH); }
	float vx = 0.5f * (static_cast<float>(nativeW) - s_width);
	float vy = 0.5f * (static_cast<float>(nativeH) - s_height);
	float vw = s_width, vh = s_height;
	if (num == 0) {
		glViewport(static_cast<GLint>(vx), static_cast<GLint>(vy), static_cast<GLint>(vw), static_cast<GLint>(vh));  // Drawable area of the window (excluding black bars)
	} else {
		// Splitscreen stereo3d
		if (nativeW == 1280 && nativeH == 1470) {  // HDMI 720p 3D mode
			glViewportIndexedf(1, 0, 750, 1280, 720);
			glViewportIndexedf(2, 0, 0, 1280, 720);
			s_width = 1280;
			s_height = 720;
		} else if (nativeW == 1920 && nativeH == 2205) {  // HDMI 1080p 3D mode
			glViewportIndexedf(1, 0, 1125, 1920, 1080);
			glViewportIndexedf(2, 0, 0, 1920, 1080);
			s_width = 1920;
			s_height = 1080;
		} else {  // Regular top/bottom 3d
			glViewportIndexedf(1, 0, vh / 2, vw, vh / 2);  // Top half of the drawable area
			glViewportIndexedf(2, 0, 0, vw, vh / 2);  // Bottom half of the drawable area
		}
	}
}

void Window::swap() {
	SDL_GL_SwapWindow(screen.get());
}

void Window::event(Uint8 const& eventID, Sint32 const& data1, Sint32 const& data2) {
	switch (eventID) {
		case SDL_WINDOWEVENT_MOVED:
			setWindowPosition(data1, data2);
			break;
		case SDL_WINDOWEVENT_MAXIMIZED:
			if (Platform::currentOS() == Platform::HostOS::OS_MAC) {
				config["graphic/fullscreen"].b() = true;
			}
			else { m_needResize = true; }
			break;
		case SDL_WINDOWEVENT_RESTORED:
			if (Platform::currentOS() == Platform::HostOS::OS_MAC) {
				config["graphic/fullscreen"].b() = false;
			}
			else {
				m_needResize = true;
			}
			break;
		case SDL_WINDOWEVENT_SHOWN:
			[[fallthrough]];
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			[[fallthrough]];
		case SDL_WINDOWEVENT_RESIZED:
			m_needResize = true;
			break;
		default: break;
	}
}

void Window::setWindowPosition(const Sint32& x, const Sint32& y)
{
	config["graphic/window_pos_x"].i() = x;
	config["graphic/window_pos_y"].i() = y;
}

FBO& Window::getFBO() {
	if (!m_fbo) {
		m_fbo = std::make_unique<FBO>(*this, s_width, (2 * s_height));
	}
	return *m_fbo;
}

void Window::setFullscreen() {
	if (m_fullscreen == config["graphic/fullscreen"].b()) return;  // We are done here
	m_fullscreen = config["graphic/fullscreen"].b();
	std::clog << "video/info: Toggle into " << (m_fullscreen ? "FULL SCREEN MODE" : "WINDOWED MODE") << std::endl;
	auto ret = SDL_SetWindowFullscreen(screen.get(), (m_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0 ));
	if (ret < 0) std::clog << "video/error: SDL_SetWindowFullscreen returned " << ret << std::endl;
	// Resize window to old size and position (if windowed now)
	if (!m_fullscreen) {
		int w = config["graphic/window_width"].i();
		int h = config["graphic/window_height"].i();
		std::clog << "video/debug: Restoring window size " << w << "x" << h << " and position " << m_windowX << "," << m_windowY << std::endl;
		SDL_SetWindowSize(screen.get(), w, h);
		SDL_SetWindowPosition(screen.get(), m_windowX, m_windowY);
	}
}

void Window::resize() {
	setFullscreen();
	if (!m_needResize) return;
	m_needResize = false;
	// Get nominal window dimensions
	int w, h;
	SDL_GetWindowSize(screen.get(), &w, &h);
	if (m_fullscreen) {
		SDL_ShowCursor(SDL_DISABLE);
		SDL_DisableScreenSaver();
	} else {
		SDL_ShowCursor(SDL_TRUE);
		SDL_EnableScreenSaver();
		config["graphic/window_width"].i() = w;
		config["graphic/window_height"].i() = h;
		SDL_GetWindowPosition(screen.get(), &m_windowX, &m_windowY);
	}
	// Get actual resolution
	int nativeW, nativeH;

	if (std::stoi(SDL_GetHint("SDL_HINT_VIDEO_HIGHDPI_DISABLED")) == 1) {
		SDL_GetWindowSize(screen.get(), &nativeW, &nativeH);
	}
	else {
		SDL_GL_GetDrawableSize(screen.get(), &nativeW, &nativeH);
	}
	s_width = static_cast<float>(nativeW);
	s_height = static_cast<float>(nativeH);
	// Enforce aspect ratio limits
	if (s_height < 0.56f * s_width) s_width = round(s_height / 0.56f);
	if (s_height > 0.8f * s_width) s_height = round(0.8f * s_width);
	std::clog << "video/info: Window size " << w << "x" << h;
	if (w != nativeW) std::clog << " (HiDPI " << nativeW << "x" << nativeH << ")";
	std::clog << ", rendering in " << s_width << "x" << s_height << std::endl;
	if (m_fbo) {
		m_fbo->resize(s_width, 2 * s_height);
	}
}

void Window::screenshot() {
	Bitmap img;
	int nativeW;
	int nativeH;
	if (std::stoi(SDL_GetHint("SDL_HINT_VIDEO_HIGHDPI_DISABLED")) == 1) {
		SDL_GetWindowSize(screen.get(), &nativeW, &nativeH);
	}
	else {
		SDL_GL_GetDrawableSize(screen.get(), &nativeW, &nativeH);
	}
	img.width = static_cast<unsigned>(nativeW);
	img.height = static_cast<unsigned>(nativeH);
	unsigned stride = (img.width * 3 + 3) & ~3u;  // Rows are aligned to 4 byte boundaries
	img.buf.resize(stride * img.height);
	img.fmt = pix::Format::RGB;
	img.linearPremul = true; // Not really, but this will use correct gamma.
	img.bottomFirst = true;
	// Get pixel data from OpenGL
	glReadPixels(0, 0, img.width, img.height, GL_RGB, GL_UNSIGNED_BYTE, img.data());
	// Compose filename with first available number
	fs::path filename;
	for (unsigned i = 1;; ++i) {
		filename = getHomeDir() / ("Performous_" + std::to_string(i) + ".png");
		if (!fs::exists(filename)) break;
	}
	// Save to disk
	writePNG(filename.string(), img, stride);
	std::clog << "video/info: Screenshot taken: " << filename << " (" << img.width << "x" << img.height << ")" << std::endl;
}

Window::SDLSystem::SDLSystem() {
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK))
		throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
}

Window::SDLSystem::~SDLSystem() {
	SDL_Quit();
}
