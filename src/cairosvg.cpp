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
CairoSVG::CairoSVG( const char * filename , unsigned int _width , unsigned int _height )
{
	cairo_t * dc;
	unsigned int height,width;

#ifdef USE_LIBSVG_CAIRO
	svg_cairo_t *scr;
	svg_cairo_create(&scr);
	svg_cairo_parse (scr, filename);
	svg_cairo_get_size (scr, &width, &height);
#endif
#ifdef USE_LIBRSVG
	RsvgHandle * svgHandle=NULL;
	GError* pError = NULL;
	RsvgDimensionData svgDimension;
	rsvg_init();
	svgHandle = rsvg_handle_new_from_file(filename,&pError);
	rsvg_handle_get_dimensions (svgHandle, &svgDimension);
	width  = svgDimension.width;
	height = svgDimension.height;
#endif

	if( _width == 0 )
		_width = width;
	if( _height == 0 )
		_height = height;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, _width, _height);
	dc = cairo_create(surface);
	cairo_scale (dc,(double) _width/width,(double)_height/height);

#ifdef USE_LIBSVG_CAIRO
	svg_cairo_render (scr, dc);
	sdl_svg=CairoToSdl::BlitToSdl(surface);
	svg_cairo_destroy (scr);
#endif
#ifdef USE_LIBRSVG
	rsvg_handle_render_cairo (svgHandle,dc);
	sdl_svg=CairoToSdl::BlitToSdl(surface);
	rsvg_handle_free (svgHandle);
#endif
	cairo_destroy (dc);
}

CairoSVG::CairoSVG( const char * data , size_t data_len ,unsigned int _width , unsigned int _height )
{
	cairo_t * dc;
	unsigned int height,width;

#ifdef USE_LIBSVG_CAIRO
	svg_cairo_t *scr;
	svg_cairo_create(&scr);
	svg_cairo_parse_buffer (scr, data, data_len);
	svg_cairo_get_size (scr, &width, &height);
#endif
#ifdef USE_LIBRSVG
	RsvgHandle * svgHandle=NULL;
	GError* pError = NULL;
	RsvgDimensionData svgDimension;
	rsvg_init();
	svgHandle = rsvg_handle_new_from_data((const guint8 *) data, data_len, &pError);
	rsvg_handle_get_dimensions (svgHandle, &svgDimension);
	width  = svgDimension.width;
	height = svgDimension.height;
#endif

	if( _width == 0 )
		_width = width;
	if( _height == 0 )
		_height = height;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, _width, _height);
	dc = cairo_create(surface);
	cairo_scale (dc,(double) _width/width,(double)_height/height);

#ifdef USE_LIBSVG_CAIRO
	svg_cairo_render (scr, dc);
	sdl_svg=CairoToSdl::BlitToSdl(surface);
	svg_cairo_destroy (scr);
#endif
#ifdef USE_LIBRSVG
	rsvg_handle_render_cairo (svgHandle,dc);
	sdl_svg=CairoToSdl::BlitToSdl(surface);
	rsvg_handle_free (svgHandle);
#endif
	cairo_destroy (dc);
}

CairoSVG::~CairoSVG( void )
{
	SDL_FreeSurface(sdl_svg);
	if(surface)
		cairo_surface_destroy(surface);
}
