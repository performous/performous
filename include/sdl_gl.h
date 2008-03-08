#ifndef __SDLGL_H__
#define __SDLGL_H__

#include "../config.h"

class SDL_GL {
  public:
	static void draw_func(float w, float h, unsigned char* surfacedata, unsigned int textureid, GLenum format, float x = 0.0f, float y = 0.0f);
	static void initTexture(int w, int h, unsigned int* textureid, GLenum format);
	static void freeTexture(unsigned int textureid);
};
#endif
