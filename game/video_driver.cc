#include "video_driver.hh"

#include "config.hh"
#include "fbo.hh"
#include "fs.hh"
#include "glmath.hh"
#include "image.hh"
#include "util.hh"
#include "joystick.hh"
#include <boost/date_time.hpp>
#include <fstream>
#include <SDL.h>


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

}

unsigned int screenW() { return s_width; }
unsigned int screenH() { return s_height; }

Window::Window(unsigned int width, unsigned int height, bool fs): m_windowW(width), m_windowH(height), m_fullscreen(fs) {
	std::atexit(SDL_Quit);
	if( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) ==  -1 ) throw std::runtime_error("SDL_Init failed");
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

	input::SDL::init(); // Joysticks etc.
	shader("surface")
	  .compileFile(getThemePath("shaders/core.vert"))
	  .compileFile(getThemePath("shaders/core.frag"), "#define SURFACE\n")
	  .link()
	  .bind()
	  .setUniformMatrix("colorMatrix", glmath::Matrix());
	shader("texture")
	  .compileFile(getThemePath("shaders/core.vert"))
	  .compileFile(getThemePath("shaders/core.frag"), "#define TEXTURE\n")
	  .link()
	  .bind()
	  .setUniformMatrix("colorMatrix", glmath::Matrix());
	shader("3dobject")
	  .compileFile(getThemePath("shaders/3dobject.vert"))
	  .compileFile(getThemePath("shaders/3dobject.frag"))
	  .link();
}

Window::~Window() { }

void Window::blank() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::render(boost::function<void (void)> drawFunc) {
	if (s_width < screen->w || s_height < screen->h) glClear(GL_COLOR_BUFFER_BIT);  // Black bars
	bool stereo = config["graphic/stereo3d"].b();
	int type = config["graphic/stereo3dtype"].i();
	if (type == 2 && !m_fullscreen) stereo = false;  // Over/under only in full screen mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (!stereo) {
		double vx = 0.5f * (screen->w - s_width);
		double vy = 0.5f * (screen->h - s_height);
		double vw = s_width, vh = s_height;
		glViewport(vx, vy, vw, vh);  // Drawable area of the window (excluding black bars)
		view(0);
		drawFunc();
		return;
	}
	// Render each eye to FBO
	unsigned w = s_width;
	unsigned h = s_height;
	if (type == 2) h /= 2;  // Half-height mode
	FBO fboleft(w, h), fboright(w, h);
	FBO* fbo[] = { &fboleft, &fboright };
	for (unsigned i = 0; i < 2; ++i) {
		UseFBO user(*fbo[i]);
		glViewport(0, 0, w, h);  // Full FBO
		view(i);
		drawFunc();
	}
	// Render to actual framebuffer from FBOs
	glViewport(0, 0, screen->w, screen->h);  // Entire window
	{
		// Use normalized eye coordinates for composition
		using namespace glmath;
		glMatrixMode(GL_PROJECTION);
		upload(Matrix());
		glMatrixMode(GL_MODELVIEW);
		upload(Matrix());
	}
	glDisable(GL_BLEND);
	Shader& sh = shader("surface");
	glmath::Matrix colorMatrix;
	for (int num = 0; num < 2; ++num) {
		if (type == 0 || type == 1) {  // Anaglyph
			{
				float saturation = 0.6;  // (0..1)
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
			if (num == 1) {
				// Right eye blends over the left eye
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
			}
			sh.bind();
		}
		{
			// Render FBO with 1:1 pixels, properly filtered/positioned for 3d
			UseTexture use(fbo[num]->getTexture());
			sh.setUniformMatrix("colorMatrix", colorMatrix);
			Dimensions dim = Dimensions().stretch(2.0 * w / screen->w, 2.0 * h / screen->h);
			if (type == 2) dim.center(num == 0 ? 0.5 : -0.5);
			fbo[num]->getTexture().draw(dim, TexCoords(0.0, 0.0, w, h));
			sh.setUniformMatrix("colorMatrix", glmath::Matrix());
		}
	}
}

bool Window::view(unsigned num) {
	// Set flags
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	// Setup the projection matrix for 2D translates
	using namespace glmath;
	glMatrixMode(GL_PROJECTION);
	float h = virtH();
	// OpenGL normalized coordinates go from -1 to 1, change scale so that our 2D translates can use the Performous normalized coordinates instead
	upload(scale(Vec3(2.0f, 2.0f / h, 1.0f)));
	// Note: we do the frustum on MODELVIEW so that 2D positioning can be done via projection matrix.
	// glTranslatef on that will move the image, not the camera (i.e. far-away and nearby objects move the same amount)
	glMatrixMode(GL_MODELVIEW);
	const float f = near_ / z0;
	upload(
	  scale(Vec3(0.5, 0.5 * h, 1.0))
	  * frustum(-0.5f * f, 0.5f * f, 0.5f * h * f, -0.5f * h * f, near_, far_)
	  * translate(Vec3(0.0, 0.0, -z0))
	);
	// Setup views
	bool stereo = config["graphic/stereo3d"].b();
	if (stereo) {
		if (num > 1) return false;
		double separation = (num == 0 ? -1 : 1) * getSeparation();
		glMatrixMode(GL_PROJECTION);
		glTranslatef(separation, 0.0f, 0.0f);
		glMatrixMode(GL_MODELVIEW);
		glTranslatef(-separation, 0.0f, 0.0f);
	} else {
		if (num != 0) return false;
	}
	return true;
}

void Window::swap() {
	SDL_GL_SwapBuffers();
}

void Window::setFullscreen(bool _fs) {
	m_fullscreen = _fs;
	resize();
}

bool Window::getFullscreen() {
	return m_fullscreen;
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


void Window::resize() {
	glutil::GLErrorChecker glerror("Window::resize");
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
		screen = SDL_SetVideoMode(width, height, 0, SDL_OPENGL | SDL_RESIZABLE | (m_fullscreen ? SDL_FULLSCREEN : 0));
		if (!screen) throw std::runtime_error(std::string("SDL_SetVideoMode failed: ") + SDL_GetError());
	}
	s_width = screen->w;
	s_height = screen->h;
	if (!m_fullscreen) {
		config["graphic/window_width"].i() = s_width;
		config["graphic/window_height"].i() = s_height;
	}
	if (s_height < 0.56f * s_width) s_width = round(s_height / 0.56f);
	if (s_height > 0.8f * s_width) s_height = round(0.8f * s_width);
}

FarTransform::FarTransform() {
	float z = far_ - 0.1f;  // Very near the far plane but just a bit closer to avoid accidental clipping
	float s = z / z0;  // Scale the image so that it looks the same size
	s *= 1.0 + 2.0 * getSeparation(); // A bit more for stereo3d (avoid black borders)
	glTranslatef(0.0f, 0.0f, -z + z0); // Very near the farplane
	glScalef(s, s, s);
}

