#include <sdl_gl.h>
#ifdef USE_OPENGL

void SDL_GL::draw_func(float w, float h, unsigned char* surfacedata, unsigned int textureid, GLenum format, float x, float y) {
	glMatrixMode(GL_MODELVIEW);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textureid);
	glPushMatrix();
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, w, h, 0, format, GL_UNSIGNED_BYTE, surfacedata);
	glTranslatef(x, y, 0.0f);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
	glTexCoord2f(w, 0.0f); glVertex2f(w, 0.0f);
	glTexCoord2f(w, h); glVertex2f(w, h);
	glTexCoord2f(0.0f, h); glVertex2f(0.0f, h);
	glEnd();
	glPopMatrix();
}

void SDL_GL::initTexture (int w, int h, unsigned int* textureid, GLenum format) {
	glGenTextures(1, textureid);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, *textureid);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, w, h, 0, format, GL_UNSIGNED_BYTE, NULL);
}

void SDL_GL::freeTexture (unsigned int textureid)
{
	glDeleteTextures (1, &textureid);
}

#endif
