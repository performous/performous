#include "video_driver.hh"

#include "config.hh"
#include "fbo.hh"
#include "fs.hh"
#include "glmath.hh"
#include "../common/image.hh"
#include "util.hh"
#include "controllers.hh"
#include "screen.hh"

#include <boost/date_time.hpp>


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
}

unsigned int screenW() { return s_width; }
unsigned int screenH() { return s_height; }

Window::Window(unsigned int width, unsigned int height, bool fs): m_windowW(width), m_windowH(height), m_fullscreen(fs), screen() {
	std::atexit(SDL_Quit);
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK))
		throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
	SDL_JoystickEventState(SDL_ENABLE);
	SDL_SetWindowTitle(screen, PACKAGE " " VERSION);
	{
		SDL_Surface* icon = SDL_LoadBMP(findFile("icon.bmp").string().c_str());
		SDL_SetWindowIcon(screen,icon);
		SDL_FreeSurface(icon);
	}

	{ // Setup GL attributes for context creation
		GLattrSetter attr_r(SDL_GL_RED_SIZE, 8);
		GLattrSetter attr_g(SDL_GL_GREEN_SIZE, 8);
		GLattrSetter attr_b(SDL_GL_BLUE_SIZE, 8);
		GLattrSetter attr_a(SDL_GL_ALPHA_SIZE, 8);
		GLattrSetter attr_buf(SDL_GL_BUFFER_SIZE, 32);
		GLattrSetter attr_d(SDL_GL_DEPTH_SIZE, 24);
		GLattrSetter attr_db(SDL_GL_DOUBLEBUFFER, 1);
		if(width < 200) width = 854; //FIXME: window should have a minimum size
		if(height < 200) height = 480;
		screen = SDL_CreateWindow(PACKAGE " " VERSION, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
			(m_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL);
		if (!screen) throw std::runtime_error(std::string("SDL_SetVideoMode failed: ") + SDL_GetError());
		SDL_GL_CreateContext(screen);

	}
	if (!m_fullscreen) {
		config["graphic/window_width"].i() = s_width;
		config["graphic/window_height"].i() = s_height;
	}
	resize();
	SDL_ShowCursor(SDL_DISABLE);

	// Dump some OpenGL info
	std::clog << "video/info: GL_VENDOR:     " << glGetString(GL_VENDOR) << std::endl;
	std::clog << "video/info: GL_VERSION:    " << glGetString(GL_VERSION) << std::endl;
	std::clog << "video/info: GL_RENDERER:   " << glGetString(GL_RENDERER) << std::endl;
	// Extensions would need more complex outputting, otherwise they will break clog.
	//std::clog << "video/info: GL_EXTENSIONS: " << glGetString(GL_EXTENSIONS) << std::endl;

	if (epoxy_gl_version() < 21) throw std::runtime_error("OpenGL 2.1 is required but not available");

	// The Stereo3D shader needs OpenGL 3.3 and GL_ARB_viewport_array, some Intel drivers support GL 3.3,
	// but not GL_ARB_viewport_array, so we just check for the extension here.
	if (epoxy_has_gl_extension("GL_ARB_viewport_array")) {
		// Compile geometry shaders when stereo is requested
		shader("color").compileFile(findFile("shaders/stereo3d.geom"));
		shader("surface").compileFile(findFile("shaders/stereo3d.geom"));
		shader("texture").compileFile(findFile("shaders/stereo3d.geom"));
		shader("3dobject").compileFile(findFile("shaders/stereo3d.geom"));
		shader("dancenote").compileFile(findFile("shaders/stereo3d.geom"));
		if (epoxy_gl_version() < 41) {
			// Enable bugfix for some older Nvidia cards
			for (ShaderMap::iterator it = m_shaders.begin(); it != m_shaders.end(); ++it) {
				Shader& sh = *it->second;
				sh.addDefines("#define ENABLE_BOGUS\n");
			}
		}
	}

	shader("color")
	  .addDefines("#define ENABLE_VERTEX_COLOR\n")
	  .compileFile(findFile("shaders/core.vert"))
	  .compileFile(findFile("shaders/core.frag"))
	  .link();
	shader("surface")
	  .addDefines("#define ENABLE_TEXTURING 1\n")
	  .compileFile(findFile("shaders/core.vert"))
	  .compileFile(findFile("shaders/core.frag"))
	  .link();
	shader("texture")
	  .addDefines("#define ENABLE_TEXTURING 2\n")
	  .addDefines("#define ENABLE_VERTEX_COLOR\n")
	  .compileFile(findFile("shaders/core.vert"))
	  .compileFile(findFile("shaders/core.frag"))
	  .link();
	shader("3dobject")
	  .addDefines("#define ENABLE_LIGHTING\n")
	  .compileFile(findFile("shaders/core.vert"))
	  .compileFile(findFile("shaders/core.frag"))
	  .link();
	shader("dancenote")
	  .addDefines("#define ENABLE_TEXTURING 2\n")
	  .addDefines("#define ENABLE_VERTEX_COLOR\n")
	  .compileFile(findFile("shaders/dancenote.vert"))
	  .compileFile(findFile("shaders/core.frag"))
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
	bool stereo = config["graphic/stereo3d"].b();
	int type = config["graphic/stereo3dtype"].i();

	if (stereo && !epoxy_has_gl_extension("GL_ARB_viewport_array")) {
		config["graphic/stereo3d"].b() = stereo = false;
		std::clog << "video/warning: Your GPU does not support Stereo3D mode (OpenGL extension ARB_viewport_array is required)" << std::endl;
		// TODO: Flash message on UI?
	}

	// Over/under only available in fullscreen
	if (stereo && type == 2 && !m_fullscreen) stereo = false;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	if (GL_EXT_framebuffer_sRGB) glEnable(GL_FRAMEBUFFER_SRGB);
	shader("color").bind();
	// Setup views (with black bars for cropping)
	int windowWidth;
	int windowHeight;
	SDL_GL_GetDrawableSize(screen, &windowWidth, &windowHeight);
	double vx = 0.5f * (windowWidth - s_width);
	double vy = 0.5f * (windowHeight - s_height);
	double vw = s_width, vh = s_height;
	if (num == 0) {
		glViewport(vx, vy, vw, vh);  // Drawable area of the window (excluding black bars)
	} else {
		// Splitscreen stereo3d
		if (windowWidth == 1280 && windowHeight == 1470) {  // HDMI 720p 3D mode
			glViewportIndexedf(1, 0, 750, 1280, 720);
			glViewportIndexedf(2, 0, 0, 1280, 720);
			s_width = 1280;
			s_height = 720;
		} else if (windowWidth == 1920 && windowHeight == 2205) {  // HDMI 1080p 3D mode
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
	SDL_GL_SwapWindow(screen);
}

void Window::setFullscreen(bool _fs) {
	if (m_fullscreen == _fs) return;
	m_fullscreen = _fs;
	SDL_SetWindowFullscreen(screen, (m_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0 ));
	resize();
}

void Window::resize() {
	int windowWidth;
	int windowHeight;
	SDL_GL_GetDrawableSize(screen, &windowWidth, &windowHeight);
	//glViewport(0, 0, windowWidth, windowHeight);
	std::clog << "video/info: Drawable size " << windowWidth << "x" << windowHeight << ", fs=" << m_fullscreen << std::endl; 
	s_width = windowWidth;
	s_height = windowHeight;
	// Enforce aspect ratio limits
	if (s_height < 0.56f * s_width) s_width = round(s_height / 0.56f);
	if (s_height > 0.8f * s_width) s_height = round(0.8f * s_width);
	if (!m_fullscreen) {
		config["graphic/window_width"].i() = s_width;
		config["graphic/window_height"].i() = s_height;
	}
}

void Window::screenshot() {
	Bitmap img;
	int windowWidth;
	int windowHeight;
	SDL_GetWindowSize(screen, &windowWidth, &windowHeight);
	img.width = windowWidth;
	img.height = windowHeight;
	unsigned stride = (img.width * 3 + 3) & ~3;  // Rows are aligned to 4 byte boundaries
	img.buf.resize(stride * img.height);
	img.fmt = pix::RGB;
	img.bottomFirst = true;
	// Get pixel data from OpenGL
	glReadPixels(0, 0, img.width, img.height, GL_RGB, GL_UNSIGNED_BYTE, img.data());
	// Compose filename from timestamp
	fs::path filename = getHomeDir() / ("Performous_" + to_iso_string(boost::posix_time::second_clock::local_time()) + ".png");
	// Save to disk
	writePNG(filename.string(), img, stride);
	std::clog << "video/info: Screenshot taken: " << filename << " (" << img.width << "x" << img.height << ")" << std::endl;
}

ColorTrans::ColorTrans(Color const& c): m_old(g_color) {
	using namespace glmath;
	g_color = g_color * mat4::diagonal(c.linear());
	Game::getSingletonPtr()->window().updateColor();
}

ColorTrans::ColorTrans(glmath::mat4 const& mat): m_old(g_color) {
	g_color = g_color * mat;
	Game::getSingletonPtr()->window().updateColor();
}

ColorTrans::~ColorTrans() {
	g_color = m_old;
	Game::getSingletonPtr()->window().updateColor();
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
	Game::getSingletonPtr()->window().updateTransforms();
}

ViewTrans::ViewTrans(glmath::mat4 const& m): m_old(g_projection) {
	g_projection = g_projection * m;
	Game::getSingletonPtr()->window().updateTransforms();
}

ViewTrans::~ViewTrans() {
	g_projection = m_old;
	Game::getSingletonPtr()->window().updateTransforms();
}

Transform::Transform(glmath::mat4 const& m): m_old(g_modelview) {
	g_modelview = g_modelview * m;
	Game::getSingletonPtr()->window().updateTransforms();
}

Transform::~Transform() {
	g_modelview = m_old;
	Game::getSingletonPtr()->window().updateTransforms();
}

glmath::mat4 farTransform() {
	float z = far_ - 0.1f;  // Very near the far plane but just a bit closer to avoid accidental clipping
	float s = z / z0;  // Scale the image so that it looks the same size
	s *= 1.0 + 2.0 * getSeparation(); // A bit more for stereo3d (avoid black borders)
	using namespace glmath;
	return translate(vec3(0.0f, 0.0f, -z + z0)) * scale(s); // Very near the farplane
}

