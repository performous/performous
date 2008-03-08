#ifndef __VIDEODRIVER_H__
#define __VIDEODRIVER_H__

#include "../config.h"
#include <boost/ptr_container/ptr_vector.hpp>
#include <vector>
#include <surface.h>

class CVideoDriver {
  public:
	CVideoDriver();
	~CVideoDriver();
	SDL_Surface* init(int width, int height, int fullscreen);

	// TODO: kill these three functions
	unsigned int initSurface(cairo_surface_t* _surf);
	void updateSurface(unsigned int _id, cairo_surface_t* _surf);
	void drawSurface(unsigned int _id, int _x=0, int _y=0);

	// ... and eventually this entire class?
	
	void blank();
	void swap();
  private:
	SDL_Surface* screen;
	boost::ptr_vector<Surface> texture_list;
};

#endif
