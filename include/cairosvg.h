#ifndef __CAIROSVG_H__
#define __CAIROSVG_H__

#include "../config.h"

class CairoSVG {
	public:
        CairoSVG( const char * filename , unsigned int _width , unsigned int _height );
  	CairoSVG( const char * data , size_t data_len ,unsigned int _width , unsigned int _height );
 	~CairoSVG( void );
	SDL_Surface * getSDLSurface(void) {return sdl_svg;};
	private:
	cairo_surface_t* surface;
	SDL_Surface *  sdl_svg;
};

#endif
