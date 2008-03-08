#ifndef __SURFACE_H__
#define __SURFACE_H__

#include "../config.h"
#include <string>
#include <boost/noncopyable.hpp>

class Surface: boost::noncopyable {
  public:
	enum Format { RGBA = 1, ARGB = 2 };
	Surface(unsigned width, unsigned height, Format format, unsigned char* buffer);
	Surface(cairo_surface_t* _surf);
	Surface(std::string filename);
	~Surface();
	void draw( float x=-0.5f, float y=-0.5f, float w=1.0f, float h=1.0f );
  private:
	void load(unsigned int width, unsigned int height, Format format, unsigned char* buffer);
	double m_width;
	double m_height;
	unsigned int texture_id;
};

#endif
