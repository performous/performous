#ifndef __SURFACE_H__
#define __SURFACE_H__

#include <stdexcept>
#include <string>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <SDL/SDL_opengl.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <cairo.h>

class Dimensions {
  public:
	Dimensions(float ar_ = 0.0f): m_ar(ar_), m_x(), m_y(), m_w(), m_h(), m_xAnchor(), m_yAnchor() {}
	Dimensions& middle(float x = 0.0f) { m_x = x; m_xAnchor = MIDDLE; return *this; }
	Dimensions& left(float x = 0.0f) { m_x = x; m_xAnchor = LEFT; return *this; }
	Dimensions& right(float x = 0.0f) { m_x = x; m_xAnchor = RIGHT; return *this; }
	Dimensions& center(float y = 0.0f) { m_y = y; m_yAnchor = CENTER; return *this; }
	Dimensions& top(float y = 0.0f) { m_y = y; m_yAnchor = TOP; return *this; }
	Dimensions& bottom(float y = 0.0f) { m_y = y; m_yAnchor = BOTTOM; return *this; }
	Dimensions& fixedWidth(float w) { m_w = w; m_h = w / m_ar; return *this; }
	Dimensions& fixedHeight(float h) { m_w = h * m_ar; m_h = h; return *this; }
	Dimensions& fitInside(float w, float h) { if (w / h > m_ar) fixedHeight(h); else fixedWidth(w); return *this; }
	Dimensions& fitOutside(float w, float h) { if (w / h > m_ar) fixedWidth(w); else fixedHeight(h); return *this; }
	Dimensions& stretch(float w, float h) { m_w = w; m_h = h; m_ar = w / h; return *this; }
	float ar() const { return m_ar; }
	float x1() const {
		switch (m_xAnchor) {
		  case LEFT: return m_x;
		  case MIDDLE: return m_x - 0.5 * m_w;
		  case RIGHT: return m_x - m_w;
		}
		throw std::logic_error("Unknown value in Dimensions::m_xAnchor");
	}
	float y1() const {
		switch (m_yAnchor) {
		  case TOP: return m_y;
		  case CENTER: return m_y - 0.5 * m_h;
		  case BOTTOM: return m_y - m_h;
		}
		throw std::logic_error("Unknown value in Dimensions::m_yAnchor");
	}
	float x2() const { return x1() + m_w; }
	float y2() const { return y1() + m_h; }
  private:
	float m_ar;
	float m_x, m_y, m_w, m_h;
	enum XAnchor { MIDDLE, LEFT, RIGHT } m_xAnchor;
	enum YAnchor { CENTER, TOP, BOTTOM } m_yAnchor;
};

struct TexCoords {
	float x1, y1, x2, y2;
	TexCoords(float x1_ = 0.0, float y1_ = 0.0, float x2_ = 1.0, float y2_ = 1.0):
	  x1(x1_), y1(y1_), x2(x2_), y2(y2_) {}
};

/** @short A RAII wrapper for allocating/deallocating OpenGL texture ID **/
template <GLenum Type> class OpenGLTexture: boost::noncopyable {
  public:
	static GLenum type() { return Type; };
	OpenGLTexture() { glGenTextures(1, &m_id); }
	~OpenGLTexture() { glDeleteTextures(1, &m_id); }
	GLuint id() const { return m_id; };
	void draw(Dimensions const& dim, TexCoords const& tex) const;
  private:
	GLuint m_id;
};

/** @short A RAII wrapper for binding to a texture (using it, modifying it) **/
class UseTexture: boost::noncopyable {
  public:
	template <GLenum Type> UseTexture(OpenGLTexture<Type> const& s): m_type(s.type()) {
		glEnable(m_type);
		glBindTexture(m_type, s.id());
	}
	~UseTexture() { glDisable(m_type); }
  private:
	GLenum m_type;
};

/** Draw the texture using the specified dimensions and texture coordinates. **/
template <GLenum Type> void OpenGLTexture<Type>::draw(Dimensions const& dim, TexCoords const& tex) const {
	UseTexture texture(*this);
	glBegin(GL_QUADS);
	glTexCoord2f(tex.x1, tex.y1); glVertex2f(dim.x1(), dim.y1());
	glTexCoord2f(tex.x2, tex.y1); glVertex2f(dim.x2(), dim.y1());
	glTexCoord2f(tex.x2, tex.y2); glVertex2f(dim.x2(), dim.y2());
	glTexCoord2f(tex.x1, tex.y2); glVertex2f(dim.x1(), dim.y2());
	glEnd();
}

namespace pix { enum Format { INT_ARGB, CHAR_RGBA, RGB, BGR }; }

/**
* @short Texture wrapper.
* Textures with non-power-of-two dimensions may be slow to load.
* If you don't need texturing, use Surface instead.
**/
class Texture: public OpenGLTexture<GL_TEXTURE_2D> {
  public:
	/** Initialize from SVG or PNG file **/
	Texture(std::string const& filename);
	/** Get aspect ratio (1.0 for square, > 1.0 for wider). **/
	float ar() const { return m_ar; }
	void load(unsigned int width, unsigned int height, pix::Format format, unsigned char const* buffer, float ar = 0.0f);
  private:
	float m_ar;
};

/**
* @short High level surface/image wrapper.
* Supports non-power-of-two dimensions, but does not support texturing, so keep tex within [0, 1].
**/
class Surface {
  public:
  	Dimensions dimensions;
	TexCoords tex;
	Surface(unsigned width, unsigned height, pix::Format format, unsigned char const* buffer);
	Surface(cairo_surface_t* _surf);
	Surface(std::string const& filename);
	void draw();
	void load(unsigned int width, unsigned int height, pix::Format format, unsigned char const* buffer, float ar = 0.0f);
  private:
	unsigned int m_width, m_height;
	OpenGLTexture<GL_TEXTURE_RECTANGLE_ARB> m_texture;
};

bool checkExtension(const char* extension);

#endif
