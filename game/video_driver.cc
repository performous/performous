#include "video_driver.hh"

#include "config.hh"
#include "glutil.hh"
#include "fs.hh"
#include "util.hh"
#include "joystick.hh"

#include <SDL.h>
#ifdef LESS_MAGIC
  #include <sstream>
  #include <fstream>
#else
  #include <Magick++.h>
#endif

#ifdef _WIN32
  // for GetTempPathA
  #include <windows.h>
#endif

namespace {
	unsigned s_width;
	unsigned s_height;
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
	SDL_EnableKeyRepeat(80, 80);
	input::SDL::init();
}

Window::~Window() { }

void Window::blank() {
	glClear(GL_COLOR_BUFFER_BIT);
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
	static unsigned int count = 0;
	unsigned width = m_fullscreen ? m_fsW : m_windowW;
	unsigned height = m_fullscreen ? m_fsH : m_windowH;

	std::vector<char> buffer(width*height*3);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, &buffer[0]);
	std::ostringstream fnstr;
#ifdef _WIN32
	char tmppath[256];
	GetTempPathA(256, tmppath);  /* this includes a backslash at the end */
	fnstr << tmppath;
#else
	fnstr << "/tmp/";
#endif
	fnstr << "performous_screenshot_" << count;
#ifdef LESS_MAGIC
	/* Write the image out as an uncompressed tga. */
	fnstr << ".tga";
	std::ofstream tgaout(fnstr.str().c_str(), std::ios::binary);
	char tga_hdr_part1[] = {
	  0x00,  /* no identification field */
	  0x00,  /* no palette */
	  0x02,  /* 24-bit RGB */
	  0x00, 0x00, 0x00, 0x00, 0x00,  /* palette info (ignored) */
	  0x00, 0x00,  /* x-origin */
	  0x00, 0x00,  /* y-origin */
	};
	tgaout.write(tga_hdr_part1, sizeof(tga_hdr_part1));
	/* two 16-bit little endian words for width and height */
	tgaout << static_cast<char>(width & 0xff);
	tgaout << static_cast<char>(width >> 8);
	tgaout << static_cast<char>(height & 0xff);
	tgaout << static_cast<char>(height >> 8);
	tgaout << static_cast<char>(24);  /* bits per pixel */
	tgaout << static_cast<char>(0x00);  /* no special flags */
	for (int i = 0; i < width*height*3; i += 3)
		std::swap(buffer[i], buffer[i+2]);  /* fix the channel order */
	tgaout.write(&buffer[0], width*height*3);  /* dump the image data */
	tgaout.close();
#else
	fnstr << ".png";
	Magick::Blob blob( &buffer[0], width*height*3);
	Magick::Image image;
	char geometry[16];
	sprintf(geometry,"%dx%d",width,height);
	image.size(geometry);
	image.depth(8);
	image.magick( "RGB" );
	image.read(blob);
	image.flip();
	image.write(fnstr.str().c_str());
#endif
	char filename[256];
	sprintf(filename,"/tmp/performous_screenshot_%u.png",count);
	image.write(filename);
	count++;
}


void Window::resize() {
	unsigned width = m_fullscreen ? m_fsW : m_windowW;
	unsigned height = m_fullscreen ? m_fsH : m_windowH;
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0);
	screen = SDL_SetVideoMode(width, height, 0, SDL_OPENGL | SDL_RESIZABLE | (m_fullscreen ? SDL_FULLSCREEN : 0));
	if (!screen) throw std::runtime_error(std::string("SDL_SetVideoMode failed: ") + SDL_GetError());

	s_width = screen->w;
	s_height = screen->h;
	if (!m_fullscreen) {
		config["graphic/window_width"].i() = s_width;
		config["graphic/window_height"].i() = s_height;
	}
	if (s_height < 0.56f * s_width) s_width = round(s_height / 0.56f);
	if (s_height > 0.8f * s_width) s_height = round(0.8f * s_width);
	glViewport(0.5f * (screen->w - s_width), 0.5f * (screen->h - s_height), s_width, s_height);
	// Set flags
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	// Set projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float h = virtH();
	// stump: under MSVC, near and far are #defined to nothing for compatibility with ancient code, hence the underscores.
	const float near_ = 1.5f; // This determines FOV: the value is your distance from the monitor (the unit being the width of the Performous window)
	const float far_ = 100.0f; // How far away can things be seen
	// Set model-view matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	const float f = 0.9f; // Avoid texture surface being exactly at the near plane (MacOSX fix)
	glFrustum(-0.5f * f, 0.5f * f, 0.5f * h * f, -0.5f * h * f, f * near_, far_);
	glTranslatef(0.0f, 0.0f, -near_);  // So that z = 0.0f is still on monitor surface

}

