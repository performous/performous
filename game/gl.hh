#pragma once

#ifdef USE_EGL
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
	// Can't include GLEW (uses regular OpenGL), so hard-code its tests (FIXME)
	#define GLEW_VERSION_2_1 GL_TRUE
	#define GLEW_VERSION_3_0 GL_FALSE
	#define GLEW_VERSION_3_3 GL_FALSE
	#define GLEW_VERSION_4_1 GL_FALSE
	#define GLEW_ARB_viewport_array GL_FALSE
	#define GLEW_EXT_texture_filter_anisotropic GL_FALSE
	// Remove EXT suffix
	#define glGenFramebuffersEXT glGenFramebuffers
	#define glFramebufferTexture2DEXT glFramebufferTexture2D
	#define glDeleteFramebuffersEXT glDeleteFramebuffers
	#define glBindFramebufferEXT glBindFramebuffer
	#define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER
	#define GL_EXT_framebuffer_sRGB GL_FALSE
	// FIXME: Need to delete all uses of GL_TEXTURE_RECTANGLE in performous
	#define GL_TEXTURE_RECTANGLE GL_TEXTURE_2D
	// GL3 stuff
	#define glViewportIndexedf
	#define GL_FRAMEBUFFER_SRGB 0x8DB9
	#define GL_SRGB_ALPHA 0x8C42
#else
	#include <GL/glew.h>
#endif
