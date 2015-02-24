#pragma once

#include "glutil.hh"
#include "../common/image.hh"
#include "video_driver.hh"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <cairo.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

/// class for geometry stuff
class Dimensions {
  public:
	/** Initialize with aspect ratio but no size, centered at screen center. **/
	Dimensions(float ar_ = 0.0f): m_ar(ar_), m_x(), m_y(), m_w(), m_h(), m_xAnchor(), m_yAnchor(), m_screenAnchor() {}
	/** Initialize with top-left corner and width & height **/
	Dimensions(float x1, float y1, float w, float h): m_ar(), m_x(x1), m_y(y1), m_w(w), m_h(h), m_xAnchor(LEFT), m_yAnchor(TOP), m_screenAnchor() {}
	/// sets middle
	Dimensions& middle(float x = 0.0f) { m_x = x; m_xAnchor = MIDDLE; return *this; }
	/// sets left
	Dimensions& left(float x = 0.0f) { m_x = x; m_xAnchor = LEFT; return *this; }
	/// sets right
	Dimensions& right(float x = 0.0f) { m_x = x; m_xAnchor = RIGHT; return *this; }
	/// sets center
	Dimensions& center(float y = 0.0f) { m_y = y; m_yAnchor = CENTER; return *this; }
	/// sets top
	Dimensions& top(float y = 0.0f) { m_y = y; m_yAnchor = TOP; return *this; }
	/// sets bottom
	Dimensions& bottom(float y = 0.0f) { m_y = y; m_yAnchor = BOTTOM; return *this; }
	/// reset aspect ratio and switch to fixed width mode
	Dimensions& ar(float ar) { m_ar = ar; return fixedWidth(m_w); }
	/// fixes width
	Dimensions& fixedWidth(float w) { m_w = w; m_h = w / m_ar; return *this; }
	/// fixes height
	Dimensions& fixedHeight(float h) { m_w = h * m_ar; m_h = h; return *this; }
	/// fits inside
	Dimensions& fitInside(float w, float h) { if (w / h > m_ar) fixedHeight(h); else fixedWidth(w); return *this; }
	/// fits outside
	Dimensions& fitOutside(float w, float h) { if (w / h > m_ar) fixedWidth(w); else fixedHeight(h); return *this; }
	/// stretches dimensions
	Dimensions& stretch(float w, float h) { m_w = w; m_h = h; m_ar = w / h; return *this; }
	/// sets screen center
	Dimensions& screenCenter(float y = 0.0f) { m_screenAnchor = CENTER; center(y); return *this; }
	/// sets screen top
	Dimensions& screenTop(float y = 0.0f) { m_screenAnchor = TOP; top(y); return *this; }
	/// sets screen bottom
	Dimensions& screenBottom(float y = 0.0f) { m_screenAnchor = BOTTOM; bottom(y); return *this; }
	/// move the object without affecting anchoring
	Dimensions& move(float x, float y) { m_x += x; m_y += y; return *this; }
	/// returns ar XXX
	float ar() const { return m_ar; }
	/// returns left
	float x1() const {
		switch (m_xAnchor) {
		  case LEFT: return m_x;
		  case MIDDLE: return m_x - 0.5 * m_w;
		  case RIGHT: return m_x - m_w;
		}
		throw std::logic_error("Unknown value in Dimensions::m_xAnchor");
	}
	/// returns top
	float y1() const {
		switch (m_yAnchor) {
		  case TOP: return screenY() + m_y;
		  case CENTER: return screenY() + m_y - 0.5 * m_h;
		  case BOTTOM: return screenY() + m_y - m_h;
		}
		throw std::logic_error("Unknown value in Dimensions::m_yAnchor");
	}
	/// returns right
	float x2() const { return x1() + w(); }
	/// returns bottom
	float y2() const { return y1() + h(); }
	/// returns x center
	float xc() const { return x1() + 0.5 * w(); }
	/// returns y center
	float yc() const { return y1() + 0.5 * h(); }
	/// returns width
	float w() const { return m_w; }
	/// returns height
	float h() const { return m_h; }

  private:
	float screenY() const;
	float m_ar;
	float m_x, m_y, m_w, m_h;
	enum XAnchor { MIDDLE, LEFT, RIGHT } m_xAnchor;
	enum YAnchor { CENTER, TOP, BOTTOM } m_yAnchor, m_screenAnchor;
};

/// texture coordinates
struct TexCoords {
	float x1, ///< left
	      y1, ///< top
	      x2, ///< right
	      y2; ///< bottom
	/// constructor
	TexCoords(float x1_ = 0.0, float y1_ = 0.0, float x2_ = 1.0, float y2_ = 1.0):
	  x1(x1_), y1(y1_), x2(x2_), y2(y2_) {}
	bool outOfBounds() const {
		return test(x1) || test(y1) || test(x2) || test(y2);
	}
private:
	static bool test(float x) { return x < 0.0 || x > 1.0; }
};

/// This function hides the ugly global vari-- I mean singleton access to ScreenManager...
Shader& getShader(std::string const& name);

/** @short A RAII wrapper for allocating/deallocating OpenGL texture ID **/
template <GLenum Type> class OpenGLTexture: boost::noncopyable {
  public:
	/// return Type
	static GLenum type() { return Type; };
	static Shader& shader() {
		switch (Type) {
		case GL_TEXTURE_2D: return getShader("texture");
		case GL_TEXTURE_RECTANGLE: return getShader("surface");
		}
		throw std::logic_error("Unknown texture type");
	}
	OpenGLTexture(): m_id() { glGenTextures(1, &m_id); }
	~OpenGLTexture() { glDeleteTextures(1, &m_id); }
	/// returns id
	GLuint id() const { return m_id; };
	/// draw in given dimensions, with given texture coordinates
	void draw(Dimensions const& dim, TexCoords const& tex = TexCoords()) const;
	/// draw a subsection of the orig dimensions, cropping by tex
	void drawCropped(Dimensions const& orig, TexCoords const& tex) const;
  private:
	GLuint m_id;
};

/** @short A RAII wrapper for binding to a texture (using it, modifying it) **/
class UseTexture: boost::noncopyable {
  public:
	/// constructor
	template <GLenum Type> UseTexture(OpenGLTexture<Type> const& tex):
	  m_shader(/* hack of the year */ (glutil::GLErrorChecker("UseTexture"), glActiveTexture(GL_TEXTURE0), glBindTexture(Type, tex.id()), tex.shader())) {}

  private:
	UseShader m_shader;
};

template <GLenum Type> void OpenGLTexture<Type>::draw(Dimensions const& dim, TexCoords const& tex) const {
	glutil::VertexArray va;

	UseTexture texture(*this);

	// The texture wraps over at the edges (repeat)
	const bool repeating = tex.outOfBounds();
	glTexParameterf(type(), GL_TEXTURE_WRAP_S, repeating ? GL_REPEAT : GL_CLAMP);
	glTexParameterf(type(), GL_TEXTURE_WRAP_T, repeating ? GL_REPEAT : GL_CLAMP);

	va.texCoord(tex.x1, tex.y1).vertex(dim.x1(), dim.y1());
	va.texCoord(tex.x2, tex.y1).vertex(dim.x2(), dim.y1());
	va.texCoord(tex.x1, tex.y2).vertex(dim.x1(), dim.y2());
	va.texCoord(tex.x2, tex.y2).vertex(dim.x2(), dim.y2());

	va.draw();
}

template <GLenum Type> void OpenGLTexture<Type>::drawCropped(Dimensions const& orig, TexCoords const& tex) const {
	Dimensions dim(
	  orig.x1() + tex.x1 * orig.w(),
	  orig.y1() + tex.y1 * orig.h(),
	  orig.w() * (tex.x2 - tex.x1),
	  orig.h() * (tex.y2 - tex.y1)
	);
	draw(dim, tex);
}

void updateSurfaces();

/**
* @short High level surface/image wrapper on top of OpenGLTexture
**/
class Surface: public OpenGLTexture<GL_TEXTURE_2D> {
public:
	struct Impl;
	/// dimensions
	Dimensions dimensions;
	/// texture coordinates
	TexCoords tex;
	Surface(): m_width(0), m_height(0), m_premultiplied(true) {}
	/// creates surface from file
	Surface(fs::path const& filename);
	~Surface();
	bool empty() const { return m_width * m_height == 0; } ///< Test if the loading has failed
	/// draws surface
	void draw() const;
	using OpenGLTexture<GL_TEXTURE_2D>::draw;
	/// loads surface into buffer
	void load(Bitmap const& bitmap);
	Shader& shader() { return m_texture.shader(); }
	unsigned width() const { return m_width; }
	unsigned height() const { return m_height; }
private:
	unsigned m_width, m_height;
	bool m_premultiplied;
	OpenGLTexture<GL_TEXTURE_2D> m_texture;
};

typedef Surface Texture;  // Backwards compatibility

/// A RAII wrapper for surface loading worker thread. There must be exactly one (global) instance whenever any Surfaces exist.
class SurfaceLoader {
public:
	SurfaceLoader();
	~SurfaceLoader();
	class Impl;
};

