#ifndef __CAIROSVG_H__
#define __CAIROSVG_H__

#include <cairo/cairo.h>
#include <SDL/SDL.h>

class CairoSVG {
	public:
	CairoSVG( const char * filename , unsigned int _width=0 , unsigned int _height=0 );
	~CairoSVG( void );
	SDL_Surface * getSDLSurface(void) {return sdl_svg;};
	private:
	cairo_surface_t* surface;
	SDL_Surface *  sdl_svg;
};

#endif
