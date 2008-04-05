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
	void resize(int width, int height) {
		const SDL_VideoInfo* videoInf = SDL_GetVideoInfo();
		screen = SDL_SetVideoMode(width, height, videoInf->vfmt->BitsPerPixel, m_videoFlags);
	}
  private:
	SDL_Surface* screen;
	boost::ptr_vector<Surface> texture_list;
	unsigned int m_videoFlags;
};

#endif
