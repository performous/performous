#ifdef USE_EGL
// We need to have this stuff separated as X11 Window would collide with our Window

#include "glutil.hh"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <iostream>
#include <stdexcept>

namespace {
	// FIXME: Move to header
	#ifdef USE_RPI
	static EGL_DISPMANX_WINDOW_T native_window;
	#else
	static XID native_window;
	#endif

	EGLConfig config;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;

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
}

// FIXME: Remove all the debug prints here
void initEGL(unsigned& s_width, unsigned& s_height) {
	EGLint num_config;
	// FIXME: Move these to Window

	std::cout << "eglGetDisplay" << std::endl;
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
	#else
	
	#endif // USE_RPI

	std::cout << "eglCreateWindowSurface" << std::endl;
	#ifdef USE_RPI
	surface = eglCreateWindowSurface(display, config, &native_window, NULL);
	#else
	surface = eglCreateWindowSurface(display, config, native_window, NULL);
	#endif

	if (surface == EGL_NO_SURFACE)
		throw std::runtime_error("eglCreateWindowSurface failed!");
	glerror.check("eglCreateWindowSurface");

	std::cout << "eglMakeCurrent" << std::endl;
	// Connect the context to the surface
	if (!eglMakeCurrent(display, surface, surface, context))
		throw std::runtime_error("eglMakeCurrent failed!");
	glerror.check("eglMakeCurrent");
	std::cout << "initEGL done" << std::endl;
}

void swapEGL() {
	eglSwapBuffers(display, surface);
}

#endif
