#ifndef __VIDEODRIVER_H__
#define __VIDEODRIVER_H__

#include "../config.h"
#include <vector>

class CVideoDriver {
  public:
	CVideoDriver();
	~CVideoDriver();
	SDL_Surface* init(int width, int height, int fullscreen);
	unsigned int initSurface(cairo_surface_t* _surf);
	void updateSurface(unsigned int _id, cairo_surface_t* _surf);
	void drawSurface(unsigned int _id, int _x=0, int _y=0);
	void drawSurface(cairo_surface_t* _surf, int _x=0, int _y=0);
	void blank();
	void swap();
  private:
	SDL_Surface* screen;
	std::vector<unsigned int> texture_list;
	std::vector<cairo_surface_t*> cairo_list;
};

#endif
