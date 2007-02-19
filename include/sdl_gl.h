#ifndef __SDLGL_H__
#define __SDLGL_H__

#include "../config.h"
#ifdef USE_OPENGL
class SDL_GL {
   public:
   static void draw_func (int _width,int _height, unsigned char* surfacedata,unsigned int textureid, GLenum format, int x=0, int y=0);
   static void initTexture (int _width,int _height,unsigned int* textureid, GLenum format);
   static void freeTexture (unsigned int textureid);
};
#endif
#endif
