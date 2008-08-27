#ifndef __SURFACE_H__
#define __SURFACE_H__

#include "../config.h"
#include <stdexcept>
#include <string>
#include <boost/noncopyable.hpp>

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
	Dimensions& fitInside(float w, float h) { if (w/h > m_ar) fixedHeight(h); else fixedWidth(w); return *this; }
	Dimensions& fitOutside(float w, float h) { if (w/h > m_ar) fixedWidth(w); else fixedHeight(h); return *this; }
	Dimensions& stretch(float w, float h) { m_w = w; m_h = h; return *this; }
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
};

class Surface: boost::noncopyable {
  public:
  	Dimensions dimensions;
	TexCoords tex;
	enum Format { INT_ARGB, CHAR_RGBA, RGB, BGR };
	enum Filetype { MAGICK = 1, SVG = 2 };
	Surface(unsigned width, unsigned height, Format format, unsigned char* buffer);
	Surface(cairo_surface_t* _surf);
	Surface(std::string filename, Filetype filetype);
	~Surface();
	void draw();
  private:
	void load(unsigned int width, unsigned int height, Format format, unsigned char* buffer, float ar = 0.0f);
	unsigned int m_width, m_height;
	GLuint texture_id;
};

#endif
