#ifndef __SURFACE_H__
#define __SURFACE_H__

#include "../config.h"
#include <string>
#include <boost/noncopyable.hpp>

class Surface: boost::noncopyable {
  public:
	enum Format { 
		INT_ARGB = 0,
		CHAR_RGBA,
		RGB,
		BGR};
	enum Filetype { MAGICK = 1, SVG = 2 };
	Surface(unsigned width, unsigned height, Format format, unsigned char* buffer);
	Surface(cairo_surface_t* _surf);
	Surface(std::string filename, Filetype filetype);
	~Surface();
	void draw(float x = 0.0f, float y = 0.0f, float w = 1.0f, float h = 0.0f);
  private:
	void load(unsigned int width, unsigned int height, Format format, unsigned char* buffer);
	float m_width;
	float m_height;
	unsigned int texture_id;
};

#endif
