#include <cairosvg.h>
#include <cairotosdl.h>
#include <svg.h>
#include <svg-cairo.h>

CairoSVG::CairoSVG( const char * filename )
{
	svg_cairo_t *scr;
	cairo_t * dc;

	svg_cairo_create(&scr);
	svg_cairo_parse (scr, filename);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 800, 600);
	dc = cairo_create(surface);
	svg_cairo_render (scr, dc);
	sdl_svg=CairoToSdl::BlitToSdl(surface);
	cairo_destroy (dc);
	svg_cairo_destroy (scr);
}

CairoSVG::~CairoSVG( void )
{
	SDL_FreeSurface(sdl_svg);
	if(surface)
		cairo_surface_destroy(surface);
}
