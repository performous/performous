#include <cairotosdl.h>

#include <iostream>

SDL_Surface* CairoToSdl::BlitToSdl(cairo_surface_t* src)
{
	static Uint32 rmask = 0x00ff0000;
        static Uint32 gmask = 0x0000ff00;
        static Uint32 bmask = 0x000000ff;
        static Uint32 amask = 0xff000000;
	
	unsigned char* cairoData = cairo_image_surface_get_data(src);
	SDL_Surface* dest = SDL_CreateRGBSurfaceFrom((void *) cairoData,
		cairo_image_surface_get_width(src),
		cairo_image_surface_get_height(src),
		32, // BPP
		cairo_image_surface_get_width(src)*4, // Stride
		rmask, gmask, bmask, amask);
	
	SDL_SetAlpha(dest, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);

	return dest;
}
