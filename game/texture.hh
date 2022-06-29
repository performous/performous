#pragma once

#include "glutil.hh"
#include "image.hh"
#include "video_driver.hh"

#include <cairo.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

/// class for geometry stuff
class Dimensions {
  public:
	/** Initialize with aspect ratio but no size, centered at screen center. **/
	Dimensions(float ar_ = 0.0f): m_ar(ar_), m_x(), m_y(), m_w(), m_h(), m_xAnchor(), m_yAnchor(), m_screenAnchor() {}
	/** Initialize with top-left corner and width & height **/
	Dimensions(float x1, float y1, float w, float h): m_ar(), m_x(x1), m_y(y1), m_w(w), m_h(h), m_xAnchor(XAnchor::LEFT), m_yAnchor(YAnchor::TOP), m_screenAnchor() {}
	/// sets middle
	Dimensions& middle(float x = 0.0f) { m_x = x; m_xAnchor = XAnchor::MIDDLE; return *this; }
	/// sets left
	Dimensions& left(float x = 0.0f) { m_x = x; m_xAnchor = XAnchor::LEFT; return *this; }
	/// sets right
	Dimensions& right(float x = 0.0f) { m_x = x; m_xAnchor = XAnchor::RIGHT; return *this; }
	/// sets center
	Dimensions& center(float y = 0.0f) { m_y = y; m_yAnchor = YAnchor::CENTER; return *this; }
	/// sets top
	Dimensions& top(float y = 0.0f) { m_y = y; m_yAnchor = YAnchor::TOP; return *this; }
	/// sets bottom
	Dimensions& bottom(float y = 0.0f) { m_y = y; m_yAnchor = YAnchor::BOTTOM; return *this; }
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
	Dimensions& screenCenter(float y = 0.0f) { m_screenAnchor = YAnchor::CENTER; center(y); return *this; }
	/// sets screen top
	Dimensions& screenTop(float y = 0.0f) { m_screenAnchor = YAnchor::TOP; top(y); return *this; }
	/// sets screen bottom
	Dimensions& screenBottom(float y = 0.0f) { m_screenAnchor = YAnchor::BOTTOM; bottom(y); return *this; }
	/// move the object without affecting anchoring
	Dimensions& move(float x, float y) { m_x += x; m_y += y; return *this; }
	/// returns ar XXX
	float ar() const { return m_ar; }
	/// returns left
	float x1() const {
		switch (m_xAnchor) {
		  case XAnchor::LEFT: return m_x;
		  case XAnchor::MIDDLE: return m_x - 0.5f * m_w;
		  case XAnchor::RIGHT: return m_x - m_w;
		}
		throw std::logic_error("Unknown value in Dimensions::m_xAnchor");
	}
	/// returns top
	float y1() const {
		switch (m_yAnchor) {
		  case YAnchor::TOP: return screenY() + m_y;
		  case YAnchor::CENTER: return screenY() + m_y - 0.5f * m_h;
		  case YAnchor::BOTTOM: return screenY() + m_y - m_h;
		}
		throw std::logic_error("Unknown value in Dimensions::m_yAnchor");
	}
	/// returns right
	float x2() const { return x1() + w(); }
	/// returns bottom
	float y2() const { return y1() + h(); }
	/// returns x center
	float xc() const { return x1() + 0.5f * w(); }
	/// returns y center
	float yc() const { return y1() + 0.5f * h(); }
	/// returns width
	float w() const { return m_w; }
	/// returns height
	float h() const { return m_h; }

  private:
	float screenY() const;
	float m_ar;
	float m_x, m_y, m_w, m_h;
	enum class XAnchor { MIDDLE, LEFT, RIGHT } m_xAnchor;
	enum class YAnchor { CENTER, TOP, BOTTOM } m_yAnchor, m_screenAnchor;
};

/// texture coordinates
struct TexCoords {
	float x1; ///< left
	float y1; ///< top
	float x2; ///< right
	float y2; ///< bottom
	/// constructor
	TexCoords(float x1_ = 0.0f, float y1_ = 0.0f, float x2_ = 1.0f, float y2_ = 1.0f):
	  x1(x1_), y1(y1_), x2(x2_), y2(y2_) {}
	bool outOfBounds() const {
		return test(x1) || test(y1) || test(x2) || test(y2);
	}
private:
	static bool test(float x) { return x < 0.0f || x > 1.0f; }
};

/// This function hides the ugly global vari-- I mean singleton access to ScreenManager...
Shader& getShader(std::string const& name);

/** @short A RAII wrapper for allocating/deallocating OpenGL texture ID **/
template <GLenum Type> class OpenGLTexture {
  public:
	OpenGLTexture(const OpenGLTexture&) = delete;
  	const OpenGLTexture& operator=(const OpenGLTexture&) = delete;
	/// return Type
	static GLenum type() { return Type; };
	static Shader& shader() { return getShader("texture"); }
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
class UseTexture {
  public:
  	UseTexture(const UseTexture&) = delete;
  	const UseTexture& operator=(const UseTexture&) = delete;
	/// constructor
	template <GLenum Type> UseTexture(OpenGLTexture<Type> const& tex):
	  m_shader(/* hack of the year */ (glutil::GLErrorChecker("UseTexture"), glActiveTexture(GL_TEXTURE0), glBindTexture(Type, tex.id()), tex.shader())) {}

  private:
	UseShader m_shader;
};

template <GLenum Type> void OpenGLTexture<Type>::draw(Dimensions const& dim, TexCoords const& tex) const {
	glutil::GLErrorChecker glerror("OpenGLTexture::draw()");
	glutil::VertexArray va;

	UseTexture texture(*this);
	glerror.check("texture");

	// The texture wraps over at the edges (repeat)
	const bool repeating = tex.outOfBounds();
	glTexParameterf(type(), GL_TEXTURE_WRAP_S, repeating ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameterf(type(), GL_TEXTURE_WRAP_T, repeating ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glerror.check("repeat mode");

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

void updateTextures();

/**
* @short High level texture/image wrapper on top of OpenGLTexture
**/
class Texture: public OpenGLTexture<GL_TEXTURE_2D> {
public:
	struct Impl;
	/// dimensions
	Dimensions dimensions;
	/// texture coordinates
	TexCoords tex;
	Texture(): m_width(0), m_height(0), m_premultiplied(true) {}
	/// creates texture from file
	Texture(fs::path const& filename);
	~Texture();
	bool empty() const { return m_width * m_height == 0; } ///< Test if the loading has failed
	/// draws texture
	void draw() const;
	using OpenGLTexture<GL_TEXTURE_2D>::draw;
	/// loads texture into buffer
	void load(Bitmap const& bitmap, bool isText = false);
	Shader& shader() { return m_texture.shader(); }
	float width() const { return m_width; }
	float height() const { return m_height; }
private:
	float m_width, m_height;
	bool m_premultiplied;
	OpenGLTexture<GL_TEXTURE_2D> m_texture;
};

/// A RAII wrapper for texture loading worker thread. There must be exactly one (global) instance whenever any Textures exist.
class TextureLoader {
public:
	TextureLoader();
	~TextureLoader();
	class Impl;
};

