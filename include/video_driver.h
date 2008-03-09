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

	// eventually kill this class?
	void blank();
	void swap();
  private:
	SDL_Surface* screen;
	boost::ptr_vector<Surface> texture_list;
};

#endif
