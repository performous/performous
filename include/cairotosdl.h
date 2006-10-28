#ifndef __CAIROTOSDL_H__
#define __CAIROTOSDL_H__

#include <cairo/cairo.h>
#include <SDL/SDL.h>

/** 
 * Functions to ease cairo-sdl-cooperation
 */
class CairoToSdl
{
	public:
	/**
	 *  Convert cairo surface to SDL surface
	 *
	 *  Creates new SDL-surface and "blits" this data from
	 *  cairo surface to it. The returned surface must bee
	 *  freed by the caller.
	 *
	 *  Note: This method uses 32-bit RGBA-colors for
	 *  easy interaction between Cairo and SDL
	 *
	 *  @param cairo_surface_t* Cairo surface to convert
	 *  @return SDL_Surface* Pointer to newly allocated SDL_Surface
	 */
	static SDL_Surface* BlitToSdl(cairo_surface_t* src);
};

#endif
