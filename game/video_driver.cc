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
	m_shader.reset(new Shader(getThemePath("shaders/core.vert"), getThemePath("shaders/core.frag"), true));
}

Window::~Window() { }

void Window::blank() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::render(boost::function<void (void)> drawFunc) {
	// Draw current frame for all the views
	FBO fbo[2];
	for (unsigned i = 0; view(i); ++i) {
		UseFBO user(fbo[i]);
		drawFunc();
	}
	{
		UseTexture use(fbo[0].getTexture());
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	//getCurrentScreen()->draw();
	//fbo[0].getTexture().draw(Dimensions(1.0).center(0.0).middle(0.0), TexCoords());
}

bool Window::view(unsigned num) {
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
	int type = config["graphic/stereo3dtype"].i();
	if (type == 1 && !m_fullscreen) stereo = false;  // Over/under only in full screen mode
	// Viewport parameters (defaults)
	double vx = 0.5f * (screen->w - s_width);
	double vy = 0.5f * (screen->h - s_height);
	double vw = s_width, vh = s_height;
	glmath::Matrix colorMatrix;
	if (stereo) {
		if (num > 1) return false;
		double separation = (num ? -1 : 1) * getSeparation();
		glMatrixMode(GL_PROJECTION);
		glTranslatef(separation, 0.0f, 0.0f);
		glMatrixMode(GL_MODELVIEW);
		glTranslatef(-separation, 0.0f, 0.0f);
		if (type == 0) {
			// FIXME: This is somewhat b0rked because what we really want is eye1 + eye2, not eye2 alpha-blended on top of eye1...
			if (!num) colorMatrix.cols[0] = Vec4(0.0, 1.0, 1.0);
			else { colorMatrix.cols[2] = colorMatrix.cols[1] = Vec4(1.0, 0.0, 0.0); colorMatrix(3,3) = 0.5; }
		}
		if (type == 1) {
			double margin = screen->h - s_height;
			vy = 0.25 * margin + (num ? 0.5 * screen->h : 0.0);
			vh *= 0.5;
		}
	} else {
		if (num != 0) return false;
	}
	m_shader->setUniformMatrix("colorMatrix", colorMatrix);
	glViewport(vx, vy, vw, vh);
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
	glerror.check("SetVideoMode");
	s_width = screen->w;
	s_height = screen->h;
	if (!m_fullscreen) {
		config["graphic/window_width"].i() = s_width;
		config["graphic/window_height"].i() = s_height;
	}
	if (s_height < 0.56f * s_width) s_width = round(s_height / 0.56f);
	if (s_height > 0.8f * s_width) s_height = round(0.8f * s_width);
	// Set flags
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
}

FarTransform::FarTransform() {
	float z = far_ - 0.1f;  // Very near the far plane but just a bit closer to avoid accidental clipping
	float s = z / z0;  // Scale the image so that it looks the same size
	s *= 1.0 + 2.0 * getSeparation(); // A bit more for stereo3d (avoid black borders)
	glTranslatef(0.0f, 0.0f, -z + z0); // Very near the farplane
	glScalef(s, s, s);
}

