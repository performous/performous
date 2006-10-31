#include <cairosvg.h>
#include <cairotosdl.h>

#ifdef USE_LIBSVG_CAIRO
  #include <svg.h>
  #include <svg-cairo.h>
#endif

#ifdef USE_LIBRSVG
  #include <librsvg/rsvg.h>
  #include <librsvg/rsvg-cairo.h>
#endif

CairoSVG::CairoSVG( const char * filename )
{
	cairo_t * dc;

#ifdef USE_LIBSVG_CAIRO
	svg_cairo_t *scr;
	svg_cairo_create(&scr);
	svg_cairo_parse (scr, filename);
#endif
#ifdef USE_LIBRSVG
	RsvgHandle * svgHandle=NULL;
	GError* pError = NULL;
	rsvg_init();
	svgHandle = rsvg_handle_new_from_file(filename,&pError);
#endif

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 800, 600);
	dc = cairo_create(surface);
#ifdef USE_LIBSVG_CAIRO
	svg_cairo_render (scr, dc);
	sdl_svg=CairoToSdl::BlitToSdl(surface);
	cairo_destroy (dc);
	svg_cairo_destroy (scr);
#endif
#ifdef USE_LIBRSVG
	rsvg_handle_render_cairo (svgHandle,dc);
	sdl_svg=CairoToSdl::BlitToSdl(surface);
	rsvg_handle_free (svgHandle);
#endif
}

CairoSVG::~CairoSVG( void )
{
	SDL_FreeSurface(sdl_svg);
	if(surface)
		cairo_surface_destroy(surface);
}
