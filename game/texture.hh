#pragma once

#include "glutil.hh"
#include "image.hh"
#include "video_driver.hh"
#include "ycoordinate.hh"

#include <cairo.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
/// class for geometry stuff
class Dimensions {
	friend class ScreenIntro;
	friend class Dialog;
  public:
	/** Initialize with aspect ratio but no size, centered at screen center. **/
	Dimensions(float ar_ = 1.0f): m_ar(ar_), m_x(), m_y(), m_w(), m_h(), nm_y(), nm_h(), m_xAnchor(), m_yAnchor(), m_screenAnchor() {
// 		std::clog << "Dimensions(ar)/debug: m_ar: " << std::to_string(m_ar) << std::endl;
	}
	/** Initialize with top-left corner and width & height **/
// 	Dimensions(float x1, YCoordinate y1, float w, YCoordinate h): m_ar(), m_x(x1), m_y(y1.raw()), m_w(w), m_h(h.raw()), m_xAnchor(LEFT), m_yAnchor(TOP), m_screenAnchor() {
// 			std::clog << "Dimensions(x1,YCoordinate y1,w,YCoordinate h)/debug: x1: " << std::to_string(x1) << ", y1: " << y1 << ", w: " << std::to_string(w) << ", h: " << h << std::endl;
// 	}
	Dimensions(float x1, float y1, float w, float h): m_ar(), m_x(x1), m_y(y1), m_w(w), m_h(h), nm_y(y1), nm_h(h), m_xAnchor(LEFT), m_yAnchor(TOP), m_screenAnchor() {
// 		std::clog << "Dimensions(x1,y1,w,h)/debug: x1: " << std::to_string(x1) << ", y1: " << std::to_string(y1) << ", w: " << std::to_string(w) << ", h: " << std::to_string(h) << std::endl;
	}
	/// sets middle
	Dimensions& middle(float x = 0.0f) { m_x = x; m_xAnchor = MIDDLE; return *this; }
	/// sets left
	Dimensions& left(float x = 0.0f) { m_x = x; m_xAnchor = LEFT; return *this; }
	/// sets right
	Dimensions& right(float x = 0.0f) { m_x = x; m_xAnchor = RIGHT; return *this; }
	/// sets center
	Dimensions& center(float y = 0.0f) { 
// 	std::clog << "dimensions/debug: inside center(), supplied y was " << std::to_string(y) << " and m_y is: " << std::to_string(m_y) <<  std::endl;
	m_yAnchor = CENTER; m_y = y; return *this; }
	/// sets top
	Dimensions& top(float y = 0.0f) {
// 	std::clog << "dimensions/debug: inside top(), supplied y was " << std::to_string(y) << " and m_y is: " << std::to_string(m_y) <<  std::endl;
	m_yAnchor = TOP; m_y = y; return *this; }
	/// sets bottom
	Dimensions& bottom(float y = 0.0f) {
// 	std::clog << "dimensions/debug: inside bottom(), supplied y was " << std::to_string(y) << " and m_y is: " << std::to_string(m_y) <<  std::endl;
	m_yAnchor = BOTTOM; m_y = y; return *this; }

	Dimensions& center(YCoordinate y) { m_yAnchor = CENTER; nm_y = y; return *this; }
	/// sets top
	Dimensions& top(YCoordinate y) { m_yAnchor = TOP; nm_y = y; return *this; }
	/// sets bottom
	Dimensions& bottom(YCoordinate y) { m_yAnchor = BOTTOM; nm_y = y; return *this; }
	/// reset aspect ratio and switch to fixed width mode
	Dimensions& ar(float ar) { m_ar = ar; return fixedWidth(m_w); }
	/// fixes width
	Dimensions& fixedWidth(float w) {
// 	std::clog << "fixedWidth/debug: w: " << std::to_string(w) << ", nm_h: " << nm_h << ", m_ar: " << std::to_string(m_ar) << ", w / m_ar: " << std::to_string(w / m_ar) << ", new nm_h: " << nm_h << std::endl;
	m_w = w; nm_h = YCoordinate(w / m_ar); return *this; }
	/// fixes height
	Dimensions& fixedHeight(float h) { m_w = h * m_ar; m_h = h; return *this; }
	Dimensions& fixedHeight(YCoordinate h) { m_w = (h.value() * m_ar); nm_h = h; return *this; }
	/// fits inside
	Dimensions& fitInside(float w, float h) { if (w / h >= m_ar) fixedHeight(h); else fixedWidth(w); return *this; }
	Dimensions& fitInside(float w, YCoordinate h) { if (w / h.value() >= m_ar) fixedHeight(h); else fixedWidth(w); return *this; }
	/// fits outside
	Dimensions& fitOutside(float w, float h) { if (w / h >= m_ar) fixedWidth(w); else fixedHeight(h); return *this; }
	Dimensions& fitOutside(float w, YCoordinate h) { if (w / h.value() >= m_ar) fixedWidth(w); else fixedHeight(h); return *this; }
	/// stretches dimensions
	Dimensions& stretch(float w, float h) { m_ar = w / h; m_w = w; nm_h = h; return *this; }
	Dimensions& stretch(float w, YCoordinate h) { m_ar = (w / h.raw()); m_w = w; nm_h = h; return *this; }
	/// scales, keeping aspect ratio.
	Dimensions& scale(float f) { m_w *= f; nm_h *= f; return *this; }
	/// sets screen center
	Dimensions& screenCenter(float y = 0.0f) { m_screenAnchor = CENTER; center(y); return *this; }
	/// sets screen top
	Dimensions& screenTop(float y = 0.0f) { m_screenAnchor = TOP; top(y); return *this; }
	/// sets screen bottom
	Dimensions& screenBottom(float y = 0.0f) { m_screenAnchor = BOTTOM; bottom(y); return *this; }
	Dimensions& screenCenter(YCoordinate y) { m_screenAnchor = CENTER; center(y); return *this; }
	Dimensions& screenTop(YCoordinate y) { m_screenAnchor = TOP; top(y); return *this; }
	Dimensions& screenBottom(YCoordinate y) { m_screenAnchor = BOTTOM; bottom(y); return *this; }
	/// move the object without affecting anchoring
	Dimensions& move(float x, float y) { m_x += x; m_y += y; return *this; }
	Dimensions& move(float x, YCoordinate y) { m_x += x; nm_y += y; return *this; }
	/// returns ar XXX
	float ar() const { return m_ar; }
	/// returns left
	float x1() const {
		switch (m_xAnchor) {
		  case LEFT: return m_x;
		  case MIDDLE: return m_x - 0.5f * m_w;
		  case RIGHT: return m_x - m_w;
		}
		throw std::logic_error("Unknown value in Dimensions::m_xAnchor");
	}
	std::string getXAnchor() const {
		switch (m_xAnchor) {
		  case LEFT: return "LEFT";
		  case MIDDLE: return "MIDDLE";
		  case RIGHT: return "RIGHT";
		}
		throw std::logic_error("Unknown value in Dimensions::m_xAnchor");		
	}
	std::string getYAnchor() const {
		switch (m_yAnchor) {
		  case TOP: return "TOP";
		  case CENTER: return "CENTER";
		  case BOTTOM: return "BOTTOM";
		}
		throw std::logic_error("Unknown value in Dimensions::m_xAnchor");		
	}
	/// returns top
	YCoordinate y1() const {
		std::string message("y1/debug: screenY(): " + std::to_string(screenY().raw()) + ", m_y: "+ std::to_string(m_y)); 
// 		{ std::clog << std::endl << message << std::endl; }
		switch (m_yAnchor) {
		  case TOP: return (screenY() + nm_y);
		  case CENTER: return (screenY() + (nm_y - 0.5f * nm_h));
		  case BOTTOM: return (screenY() + (nm_y - nm_h));
		}
		throw std::logic_error("Unknown value in Dimensions::m_yAnchor");
	}
	/// returns right
	float x2() const { return x1() + w(); }
	/// returns bottom
	YCoordinate y2() const { return (y1() + h()); }
	/// returns x center
	float xc() const { return x1() + 0.5f * w(); }
// 	p = x1 + 0.5f * 0.96f
	/// returns y center
	YCoordinate yc() const { return y1() + (0.5f * h()); }
	/// returns width
	float w() const { 
// 		if (m_w == 0.0f) {
// 			switch (m_xAnchor) {
// 				case LEFT: return std::abs(0.48f - m_x);
// 				case MIDDLE: return std::abs(0.0f - m_x * 0.5f);
// 				case RIGHT: return std::abs(-0.48f - m_x);
// 			}
// 		}
		return m_w;
	}
	/// returns height
	YCoordinate const& h() const { return nm_h; }
	float getAbsX() const { return m_x; }
  private:
	YCoordinate screenY() const;
	float m_ar;
	float m_x, m_y, m_w, m_h;
	YCoordinate nm_y, nm_h;
	enum XAnchor { MIDDLE, LEFT, RIGHT } m_xAnchor;
	enum YAnchor { CENTER, TOP, BOTTOM } m_yAnchor, m_screenAnchor;
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
	std::clog << "Texture::draw(Dimensions const&, TexCoords const&)/debug: tex.x1: " << std::to_string(tex.x1) << ", tex.y1: " << std::to_string(tex.y1) << ", tex.x2: " << std::to_string(tex.x2) << ", tex.y2: " << std::to_string(tex.y2) << ", dim.x1(): " << std::to_string(dim.x1()) << ", dim.x2(): " << std::to_string(dim.x2()) << ", dim.y1(): " << std::string(dim.y1()) << ", dim.y2(): " << std::string(dim.y1()) << std::endl;
	va.texCoord(tex.x1, tex.y1).vertex(dim.x1(), dim.y1().value());
	va.texCoord(tex.x2, tex.y1).vertex(dim.x2(), dim.y1().value());
	va.texCoord(tex.x1, tex.y2).vertex(dim.x1(), dim.y2().value());
	va.texCoord(tex.x2, tex.y2).vertex(dim.x2(), dim.y2().value());
	va.draw();
}

template <GLenum Type> void OpenGLTexture<Type>::drawCropped(Dimensions const& orig, TexCoords const& tex) const {
	Dimensions dim(
	  orig.x1() + tex.x1 * orig.w(),
	  YCoordinate(orig.y1() + tex.y1 * orig.h()).raw(),
	  orig.w() * (tex.x2 - tex.x1),
	  YCoordinate(orig.h() * (tex.y2 - tex.y1)).raw()
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
	Texture(): m_width(0.0f), m_height(0.0f), m_premultiplied(true) {}
	/// creates texture from file
	Texture(fs::path const& filename);
	~Texture();
	bool empty() const { return m_width * m_height == 0.0f; } ///< Test if the loading has failed
	/// draws texture
	void draw() const;
	using OpenGLTexture<GL_TEXTURE_2D>::draw;
	/// loads texture into buffer
	void load(Bitmap const& bitmap, bool isText = false);
	Shader& shader() { return m_texture.shader(); }
	float width() const { return m_width; }
	float height() const { return m_height; }
private:
	std::string m_filename;
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

