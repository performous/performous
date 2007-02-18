#include <sdl_gl.h>
#ifdef USE_OPENGL
void SDL_GL::init_gl ()
{
#ifdef DEBUG
        printf ("OpenGL version: %s\n", glGetString (GL_VERSION));
        printf ("OpenGL vendor: %s\n", glGetString (GL_VENDOR));
        printf ("OpenGL renderer: %s\n", glGetString (GL_RENDERER));
#endif
        glClearColor (1.0f, 1.0f, 1.0f, 1.0f);
        glDisable (GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable (GL_BLEND);
        glEnable (GL_TEXTURE_RECTANGLE_ARB);
}

void SDL_GL::draw_func (int _width,int _height, unsigned char* surfacedata, unsigned int textureid, GLenum format) {

        glMatrixMode (GL_MODELVIEW);
        glLoadIdentity ();
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glPushMatrix ();

        glBindTexture (GL_TEXTURE_RECTANGLE_ARB, textureid);
        glTexImage2D (GL_TEXTURE_RECTANGLE_ARB,
                                  0,
                                  GL_RGBA,
                                  _width,
                                  _height,
                                  0,
                                  format,
                                  GL_UNSIGNED_BYTE,
                                  surfacedata);

        glBegin (GL_QUADS);
        glTexCoord2f (0.0f, 0.0f);
        glVertex2f (0, 0);
        glTexCoord2f ((GLfloat) _width, 0.0f);
        glVertex2f (1.0f, 0.0f);
        glTexCoord2f ((GLfloat) _width, (GLfloat) _height);
        glVertex2f (1.0f, 1.0f);
        glTexCoord2f (0.0f, (GLfloat) _height);
        glVertex2f (0.0f, 1.0f);

        glEnd ();

        glPopMatrix ();
}

void SDL_GL::initTexture (int _width,int _height,unsigned int* textureid, GLenum format) {
        
        glViewport (0, 0, _width, _height);
        glMatrixMode (GL_PROJECTION);
        glLoadIdentity ();
        glOrtho (0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f);

        glClear (GL_COLOR_BUFFER_BIT);

        glDeleteTextures (1, textureid);
        glGenTextures (1, textureid);
        glBindTexture (GL_TEXTURE_RECTANGLE_ARB, *textureid);
        glTexImage2D (GL_TEXTURE_RECTANGLE_ARB,
                                  0,
                                  GL_RGBA,
                                  _width,
                                  _height,
                                  0,
                                  format,
                                  GL_UNSIGNED_BYTE,
                                  NULL);
}
#endif
