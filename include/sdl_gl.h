#ifndef __SDLGL_H__
#define __SDLGL_H__

#include "../config.h"
#ifdef USE_OPENGL
class SDL_GL {
   public:
   static void init_gl();
   static void draw_func (int _width,int _height, unsigned char* surfacedata,unsigned int textureid, GLenum format);
   static void initTexture (int _width,int _height,unsigned int* textureid, GLenum format);
};
#endif
#endif
