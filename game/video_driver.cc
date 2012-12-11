#include "video_driver.hh"

#include "config.hh"
#include "fbo.hh"
#include "fs.hh"
#include "glmath.hh"
#include "image.hh"
#include "util.hh"
#include "controllers.hh"
#include "screen.hh"
#include <boost/date_time.hpp>
#include <fstream>
#include <SDL.h>
#ifdef USE_RPI
# include "bcm_host.h"
#endif
#ifdef USE_EGL
# include <EGL/egl.h>
# include <EGL/eglext.h>
#endif

#ifndef GLEW_ARB_viewport_array
# define GLEW_ARB_viewport_array GL_FALSE
# define glViewportIndexedf(index, x, y,  w,  h) {}
# warning "Your version of GLEW is too old, Performous is smart and let you compile it anyway"
#endif
#ifndef GLEW_VERSION_3_3
# define GLEW_VERSION_3_3 GL_FALSE
# warning "Your version of GLEW is too old, Performous is smart and let you compile it anyway"
#endif
#ifndef GLEW_VERSION_4_1
# define GLEW_VERSION_4_1 GL_FALSE
# warning "Your version of GLEW is too old, Performous is smart and let you compile it anyway"
#endif

/*
/home/yoda/performous/game/video_driver.cc: In member function âid Window::render(boost::function<void()>)â/home/yoda/performous/game/video_driver.cc:193:43: error: âViewportIndexedfâas not declared in this scope
/home/yoda/performous/game/video_driver.cc: In member function âid Window::view(unsigned int)â/home/yoda/performous/game/video_driver.cc:256:46: error: âViewportIndexedfâas not declared in this scope
*/


namespace {
	unsigned s_width;
	unsigned s_height;
	/// Attempt to set attribute and report errors.
	/// Tests for success when destoryed.
	struct GLattrSetter {
		GLattrSetter(SDL_GLattr attr, int value): m_attr(attr), m_value(value) {
			if (SDL_GL_SetAttribute(attr, value)) std::clog << "video/warning: Error setting GLattr " << m_attr << std::endl;
		}
		~GLattrSetter() {
			int value;
			SDL_GL_GetAttribute(m_attr, &value);
			if (value != m_value)
				std::clog << "video/warning: Error setting GLattr " << m_attr
				<< ": requested " << m_value << ", got " << value << std::endl;
		}
		SDL_GLattr m_attr;
		int m_value;
	};

	double getSeparation() {
		return config["graphic/stereo3d"].b() ? 0.001f * config["graphic/stereo3dseparation"].f() : 0.0;
	}

	// stump: under MSVC, near and far are #defined to nothing for compatibility with ancient code, hence the underscores.
	const float near_ = 0.1f; // This determines the near clipping distance (must be > 0)
	const float far_ = 110.0f; // How far away can things be seen
	const float z0 = 1.5f; // This determines FOV: the value is your distance from the monitor (the unit being the width of the Performous window)

	glmath::mat4 g_color = glmath::mat4::identity();
	glmath::mat4 g_projection = glmath::mat4::identity();
	glmath::mat4 g_modelview =  glmath::mat4::identity();

	// FIXME: Move to header
	#ifdef USE_RPI
	static EGL_DISPMANX_WINDOW_T native_window;
	#endif

	#ifdef USE_EGL
	static const EGLint egl_attributes[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};

	static const EGLint egl_context_attributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	#endif
}

unsigned int screenW() { return s_width; }
unsigned int screenH() { return s_height; }

// FIXME: Window member?
// FIXME: Remove all the debug prints here
void initEGL() {
#ifdef USE_EGL
	int32_t success = 0;
	EGLint num_config;
	// FIXME: Move these to Window
	EGLConfig config;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;

	std::cout << "initEGL" << std::endl;
	// Get an EGL display connection
	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (display == EGL_NO_DISPLAY)
		throw std::runtime_error("eglGetDisplay failed!");
	glutil::GLErrorChecker glerror("initEGL");

	std::cout << "eglInitialize" << std::endl;
	if (!eglInitialize(display, NULL, NULL))
		throw std::runtime_error("eglInitialize failed!");
	glerror.check("eglInitialize");

	std::cout << "eglChooseConfig" << std::endl;
	// Get EGL frame buffer configuration
	if (!eglChooseConfig(display, egl_attributes, &config, 1, &num_config))
		throw std::runtime_error("eglChooseConfig failed!");
	glerror.check("eglChooseConfig");

	std::cout << "eglBindAPI" << std::endl;
	if (!eglBindAPI(EGL_OPENGL_ES_API))
		throw std::runtime_error("eglBindAPI failed!");
	glerror.check("eglBindAPI");

	std::cout << "eglCreateContext" << std::endl;
	// Create the EGL rendering context
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, egl_context_attributes);
	if (context == EGL_NO_CONTEXT)
		throw std::runtime_error("eglCreateContext failed!");
	glerror.check("eglCreateContext");

	#ifdef USE_RPI
	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;

	std::cout << "graphics_get_display_size" << std::endl;
	// Create EGL window surface
	if (graphics_get_display_size(0 /* LCD */, &s_width, &s_height) < 0)
		throw std::runtime_error("graphics_get_display_size failed!");

	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = s_width;
	dst_rect.height = s_height;

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = s_width << 16;
	src_rect.height = s_height << 16;

	dispman_display = vc_dispmanx_display_open(0 /*LCD*/);
	dispman_update = vc_dispmanx_update_start(0);
	dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display,
		0 /*layer*/, &dst_rect, 0 /*src*/, &src_rect,
		DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0 /*clamp*/, DISPMANX_NO_ROTATE /*transform*/);

	native_window.element = dispman_element;
	native_window.width = s_width;
	native_window.height = s_height;
	vc_dispmanx_update_submit_sync(dispman_update);
	glerror.check("vc_dispmanx_update_submit_sync");
	#endif // USE_RPI

	std::cout << "eglCreateWindowSurface" << std::endl;
	surface = eglCreateWindowSurface(display, config, &native_window, NULL);
	if (surface == EGL_NO_SURFACE)
		throw std::runtime_error("eglCreateWindowSurface failed!");
	glerror.check("eglCreateWindowSurface");

	std::cout << "eglMakeCurrent" << std::endl;
	// Connect the context to the surface
	if (!eglMakeCurrent(display, surface, surface, context))
		throw std::runtime_error("eglMakeCurrent failed!");
	glerror.check("eglMakeCurrent");
	std::cout << "initEGL done" << std::endl;
#endif // USE_EGL
}

Window::Window(unsigned int width, unsigned int height, bool fs): m_windowW(width), m_windowH(height), m_fullscreen(fs), screen() {
	std::atexit(SDL_Quit);
	#ifdef USE_RPI
	std::cout << "Attempting to initialize Raspberry Pi VideoCore" << std::endl;
	bcm_host_init();
	std::atexit(bcm_host_deinit);
	std::cout << "Attempting to initialize SDL" << std::endl;
	#endif
	if( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) == -1 ) throw std::runtime_error("SDL_Init failed");
	SDL_WM_SetCaption(PACKAGE " " VERSION, PACKAGE);
	{
		SDL_Surface* icon = SDL_LoadBMP(getThemePath("icon.bmp").c_str());
		SDL_WM_SetIcon(icon, NULL);
		SDL_FreeSurface(icon);
	}
	// SDL_SetVideoMode not called yet => get the desktop resolution for fs mode
	SDL_VideoInfo const* vi = SDL_GetVideoInfo();
	m_fsW = vi->current_w;
	m_fsH = vi->current_h;
	std::cout << "Attempting to set video mode" << std::endl;
	resize();
	SDL_ShowCursor(SDL_DISABLE);
	SDL_EnableUNICODE(SDL_ENABLE);
	if (glewInit() != GLEW_OK) throw std::runtime_error("Initializing GLEW failed (is your OpenGL broken?)");
	// Dump some OpenGL info
	std::clog << "video/info: GL_VENDOR:     " << glGetString(GL_VENDOR) << std::endl;
	std::clog << "video/info: GL_VERSION:    " << glGetString(GL_VERSION) << std::endl;
	std::clog << "video/info: GL_RENDERER:   " << glGetString(GL_RENDERER) << std::endl;
	// Extensions would need more complex outputting, otherwise they will break clog.
	//std::clog << "video/info: GL_EXTENSIONS: " << glGetString(GL_EXTENSIONS) << std::endl;

	if (!GLEW_VERSION_2_1) throw std::runtime_error("OpenGL 2.1 is required but not available");

	if (!GLEW_ARB_viewport_array && config["graphic/stereo3d"].b()) throw std::runtime_error("OpenGL extension ARB_viewport_array is required but not available when using stereo mode");

	input::SDL::init(); // Joysticks etc.

	if (GLEW_VERSION_3_3) {
		// Compile geometry shaders when stereo is requested
		shader("color").compileFile(getThemePath("shaders/stereo3d.geom"));
		shader("surface").compileFile(getThemePath("shaders/stereo3d.geom"));
		shader("texture").compileFile(getThemePath("shaders/stereo3d.geom"));
		shader("3dobject").compileFile(getThemePath("shaders/stereo3d.geom"));
		shader("dancenote").compileFile(getThemePath("shaders/stereo3d.geom"));
		if (!GLEW_VERSION_4_1) {
			// Enable bugfix for some older Nvidia cards
			for (ShaderMap::iterator it = m_shaders.begin(); it != m_shaders.end(); ++it) {
				Shader& sh = *it->second;
				sh.addDefines("#define ENABLE_BOGUS\n");
			}
		}
	}

	shader("color")
	  .addDefines("#define ENABLE_VERTEX_COLOR\n")
	  .compileFile(getThemePath("shaders/core.vert"))
	  .compileFile(getThemePath("shaders/core.frag"))
	  .link();
	shader("surface")
	  .addDefines("#define ENABLE_TEXTURING 1\n")
	  .compileFile(getThemePath("shaders/core.vert"))
	  .compileFile(getThemePath("shaders/core.frag"))
	  .link();
	shader("texture")
	  .addDefines("#define ENABLE_TEXTURING 2\n")
	  .addDefines("#define ENABLE_VERTEX_COLOR\n")
	  .compileFile(getThemePath("shaders/core.vert"))
	  .compileFile(getThemePath("shaders/core.frag"))
	  .link();
	shader("3dobject")
	  .addDefines("#define ENABLE_LIGHTING\n")
	  .compileFile(getThemePath("shaders/core.vert"))
	  .compileFile(getThemePath("shaders/core.frag"))
	  .link();
	shader("dancenote")
	  .addDefines("#define ENABLE_TEXTURING 2\n")
	  .addDefines("#define ENABLE_VERTEX_COLOR\n")
	  .compileFile(getThemePath("shaders/dancenote.vert"))
	  .compileFile(getThemePath("shaders/core.frag"))
	  .link();

	updateColor();
	view(0);  // For loading screens
}

Window::~Window() { }

void Window::blank() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::updateStereo(float sepFactor) {
	for (ShaderMap::iterator it = m_shaders.begin(); it != m_shaders.end(); ++it) {
		Shader& sh = *it->second;
		sh.bind();
		try {
			sh["sepFactor"].set(sepFactor);
			sh["z0"].set(z0 - 2.0f * near_);  // Why minus two times zNear, I have no idea -Tronic
		} catch(...) {}  // Not fatal if 3d shader is missing
	}
}

void Window::updateColor() {
	for (ShaderMap::iterator it = m_shaders.begin(); it != m_shaders.end(); ++it) {
		Shader& sh = *it->second;
		sh["colorMatrix"].setMat4(g_color);
	}
}

void Window::updateTransforms() {
	using namespace glmath;
	mat3 normal(g_modelview);
	for (ShaderMap::iterator it = m_shaders.begin(); it != m_shaders.end(); ++it) {
		Shader& sh = *it->second;
		sh.bind();
		sh["projMatrix"].setMat4(g_projection);
		sh["mvMatrix"].setMat4(g_modelview);
		try {
			sh["normalMatrix"].setMat3(normal);
		} catch(...) {}  // Not fatal if normalMatrix is missing (only 3d objects use it)
	}
}

void Window::render(boost::function<void (void)> drawFunc) {
	glutil::GLErrorChecker glerror("Window::render");
	ViewTrans trans;  // Default frustum
	if (s_width < screen->w || s_height < screen->h) glClear(GL_COLOR_BUFFER_BIT);  // Black bars
	bool stereo = config["graphic/stereo3d"].b();
	int type = config["graphic/stereo3dtype"].i();

	// Over/under only available in fullscreen
	if (stereo && type == 2 && !m_fullscreen) stereo = false;

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	updateStereo(stereo ? getSeparation() : 0.0);
	glerror.check("setup");
	// Can we do direct to framebuffer rendering (no FBO)?
	if (!stereo || type == 2) { view(stereo); drawFunc(); return; }
	// Render both eyes to FBO (full resolution top/bottom for anaglyph)
	unsigned w = s_width;
	unsigned h = 2 * s_height;
	FBO fbo(w, h);
	glerror.check("FBO");
	{
		UseFBO user(fbo);
		view(0);
		glViewportIndexedf(1, 0, h / 2, w, h / 2);
		glViewportIndexedf(2, 0, 0, w, h / 2);
		drawFunc();
	}
	glerror.check("Render to FBO");
	// Render to actual framebuffer from FBOs
	UseTexture use(fbo.getTexture());
	view(0);  // Viewport for drawable area
	glDisable(GL_BLEND);
	glmath::mat4 colorMatrix = glmath::mat4::identity();
	updateStereo(0.0);  // Disable stereo mode while we composite
	glerror.check("FBO->FB setup");
	for (int num = 0; num < 2; ++num) {
		{
			float saturation = 0.5;  // (0..1)
			float col = (1.0 + 2.0 * saturation) / 3.0;
			float gry = 0.5 * (1.0 - col);
			bool out[3] = {};  // Which colors to output
			if (type == 0 && num == 0) { out[0] = true; }  // Red
			if (type == 0 && num == 1) { out[1] = out[2] = true; }  // Cyan
			if (type == 1 && num == 0) { out[1] = true; }  // Green
			if (type == 1 && num == 1) { out[0] = out[2] = true; }  // Magenta
			for (unsigned i = 0; i < 3; ++i) {
				for (unsigned j = 0; j < 3; ++j) {
					double val = 0.0;
					if (out[i]) val = (i == j ? col : gry);
					colorMatrix(i, j) = val;
				}
			}
		}
		// Render FBO with 1:1 pixels, properly filtered/positioned for 3d
		ColorTrans c(colorMatrix);
		Dimensions dim = Dimensions(double(w) / h).fixedWidth(1.0);
		dim.center((num == 0 ? 0.25 : -0.25) * dim.h());
		if (num == 1) {
			// Right eye blends over the left eye
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
		}
		fbo.getTexture().draw(dim, TexCoords(0.0, h, w, 0));
	}
	glerror.check("FBO->FB postcondition");
}

void Window::view(unsigned num) {
	glutil::GLErrorChecker glerror("Window::view");
	// Set flags
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	if (GL_EXT_framebuffer_sRGB) glEnable(GL_FRAMEBUFFER_SRGB);
	shader("color").bind();
	// Setup views (with black bars for cropping)
	double vx = 0.5f * (screen->w - s_width);
	double vy = 0.5f * (screen->h - s_height);
	double vw = s_width, vh = s_height;
	if (num == 0) {
		glViewport(vx, vy, vw, vh);  // Drawable area of the window (excluding black bars)
	} else {
		// Splitscreen stereo3d
		if (screen->w == 1280 && screen->h == 1470) {  // HDMI 720p 3D mode
			glViewportIndexedf(1, 0, 750, 1280, 720);
			glViewportIndexedf(2, 0, 0, 1280, 720);
			s_width = 1280;
			s_height = 720;
		} else if (screen->w == 1920 && screen->h == 2205) {  // HDMI 1080p 3D mode
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
	SDL_GL_SwapBuffers();
}

void Window::setFullscreen(bool _fs) {
	if (m_fullscreen == _fs) return;
	m_fullscreen = _fs;
	resize();
}

void Window::resize() {
	unsigned width = m_fullscreen ? m_fsW : m_windowW;
	unsigned height = m_fullscreen ? m_fsH : m_windowH;
	{ // Setup GL attributes for context creation
		GLattrSetter attr_r(SDL_GL_RED_SIZE, 8);
		GLattrSetter attr_g(SDL_GL_GREEN_SIZE, 8);
		GLattrSetter attr_b(SDL_GL_BLUE_SIZE, 8);
		GLattrSetter attr_a(SDL_GL_ALPHA_SIZE, 8);
		GLattrSetter attr_buf(SDL_GL_BUFFER_SIZE, 32);
		GLattrSetter attr_d(SDL_GL_DEPTH_SIZE, 24);
		GLattrSetter attr_s(SDL_GL_STENCIL_SIZE, 8);
		GLattrSetter attr_db(SDL_GL_DOUBLEBUFFER, 1);
		GLattrSetter attr_ar(SDL_GL_ACCUM_RED_SIZE, 0);
		GLattrSetter attr_ag(SDL_GL_ACCUM_GREEN_SIZE, 0);
		GLattrSetter attr_ab(SDL_GL_ACCUM_BLUE_SIZE, 0);
		GLattrSetter attr_aa(SDL_GL_ACCUM_ALPHA_SIZE, 0);
		SDL_FreeSurface(screen);
		#ifdef USE_EGL
		std::cout << "Attempting SDL_SetVideoMode" << std::endl;
		//screen = SDL_SetVideoMode(0, 0, 32, SDL_SWSURFACE);
		screen = new SDL_Surface;
		screen->w = s_width;
		screen->h = s_height;
		initEGL();
		#else
		screen = SDL_SetVideoMode(width, height, 0, SDL_OPENGL | (m_fullscreen ? SDL_FULLSCREEN : SDL_RESIZABLE));
		#endif
		if (!screen) throw std::runtime_error(std::string("SDL_SetVideoMode failed: ") + SDL_GetError());
	}
	s_width = screen->w;
	s_height = screen->h;
	if (!m_fullscreen) {
		config["graphic/window_width"].i() = s_width;
		config["graphic/window_height"].i() = s_height;
	}
	// Enforce aspect ratio limits
	if (s_height < 0.56f * s_width) s_width = round(s_height / 0.56f);
	if (s_height > 0.8f * s_width) s_height = round(0.8f * s_width);
}

void Window::screenshot() {
	Image img;
	img.w = m_fullscreen ? m_fsW : m_windowW;
	img.h = m_fullscreen ? m_fsH : m_windowH;
	img.data.resize(((img.w + 3) & ~3) * img.h * 3);
	img.format = pix::RGB;
	img.reverse = true;
	// Get pixel data from OpenGL
	glReadPixels(0, 0, img.w, img.h, GL_RGB, GL_UNSIGNED_BYTE, &img.data[0]);
	// Compose filename from timestamp
	fs::path filename = getHomeDir() / ("Performous_" + to_iso_string(boost::posix_time::second_clock::local_time()) + ".png");
	// Save to disk
	writePNG(filename.string(), img);
	std::clog << "video/info: Screenshot taken: " << filename << " (" << img.w << "x" << img.h << ")" << std::endl;
}

ColorTrans::ColorTrans(Color const& c): m_old(g_color) {
	using namespace glmath;
	g_color = g_color * mat4::diagonal(c.linear());
	ScreenManager::getSingletonPtr()->window().updateColor();
}

ColorTrans::ColorTrans(glmath::mat4 const& mat): m_old(g_color) {
	g_color = g_color * mat;
	ScreenManager::getSingletonPtr()->window().updateColor();
}

ColorTrans::~ColorTrans() {
	g_color = m_old;
	ScreenManager::getSingletonPtr()->window().updateColor();
}

ViewTrans::ViewTrans(double offsetX, double offsetY, double frac): m_old(g_projection) {
	// Setup the projection matrix for 2D translates
	using namespace glmath;
	double h = virtH();
	const double f = near_ / z0;  // z0 to nearplane conversion factor
	// Corners of the screen at z0
	double x1 = -0.5, x2 = 0.5;
	double y1 = 0.5 * h, y2 = -0.5 * h;
	// Move the perspective point by frac of offset (i.e. move the image)
	double persX = frac * offsetX, persY = frac * offsetY;
	x1 -= persX; x2 -= persX;
	y1 -= persY; y2 -= persY;
	// Perspective projection + the rest of the offset in eye (world) space
	g_projection = frustum(f * x1, f * x2, f * y1, f * y2, near_, far_)
	  * translate(vec3(offsetX - persX, offsetY - persY, -z0));
	ScreenManager::getSingletonPtr()->window().updateTransforms();
}

ViewTrans::ViewTrans(glmath::mat4 const& m): m_old(g_projection) {
	g_projection = g_projection * m;
	ScreenManager::getSingletonPtr()->window().updateTransforms();
}

ViewTrans::~ViewTrans() {
	g_projection = m_old;
	ScreenManager::getSingletonPtr()->window().updateTransforms();
}

Transform::Transform(glmath::mat4 const& m): m_old(g_modelview) {
	g_modelview = g_modelview * m;
	ScreenManager::getSingletonPtr()->window().updateTransforms();
}

Transform::~Transform() {
	g_modelview = m_old;
	ScreenManager::getSingletonPtr()->window().updateTransforms();
}

glmath::mat4 farTransform() {
	float z = far_ - 0.1f;  // Very near the far plane but just a bit closer to avoid accidental clipping
	float s = z / z0;  // Scale the image so that it looks the same size
	s *= 1.0 + 2.0 * getSeparation(); // A bit more for stereo3d (avoid black borders)
	using namespace glmath;
	return translate(vec3(0.0f, 0.0f, -z + z0)) * scale(s); // Very near the farplane
}

