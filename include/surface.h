#ifndef __SURFACE_H__
#define __SURFACE_H__

#include "../config.h"
#include <string>
#include <boost/noncopyable.hpp>

#define SURFACE_RGBA 1
#define SURFACE_ARGB 2
#define FILE_SVG     1
#define FILE_MAGICK  2

class Surface: boost::noncopyable {
	public:
	Surface(double width, double height, unsigned int format, unsigned char * buffer);
	Surface(std::string filename, unsigned int type );
	~Surface();
	void draw( float x=-0.5f, float y=-0.5f, float w=1.0f, float h=1.0f );
	private:
	void glLoad(unsigned char * buffer);
	void glUnload();
	double m_width;
	double m_height;
	unsigned int m_format;
	unsigned int texture_id;
};

#endif
