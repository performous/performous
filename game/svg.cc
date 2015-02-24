#include "svg.hh"

#include "cache.hh"
#include "configuration.hh"
#include "../common/image.hh"

#include <librsvg/rsvg.h>
#include <iostream>

// Avoid deprecation messages with new versions since Ubuntu 12.10.
#if LIBRSVG_MAJOR_VERSION * 10000 + LIBRSVG_MINOR_VERSION * 100 + LIBRSVG_MICRO_VERSION < 23602
#include <librsvg/rsvg-cairo.h>
#endif

void loadSVG(Bitmap& bitmap, fs::path const& filename) {
	double factor = config["graphic/svg_lod"].f();
	// Try to load a cached PNG instead
	if (cache::loadSVG(bitmap, filename, factor)) return;
	std::clog << "image/debug: Loading SVG: " + filename.string() << std::endl;
	// Open the SVG file in librsvg
#if !GLIB_CHECK_VERSION(2, 36, 0)   // Avoid deprecation warnings
	g_type_init();
#endif
	GError* pError = NULL;
	boost::shared_ptr<RsvgHandle> svgHandle(rsvg_handle_new_from_file(filename.string().c_str(), &pError), g_object_unref);
	if (pError) {
		g_error_free(pError);
		throw std::runtime_error("Unable to load " + filename.string());
	}
	// Get SVG dimensions
	RsvgDimensionData svgDimension;
	rsvg_handle_get_dimensions(svgHandle.get(), &svgDimension);
	// Prepare the pixel buffer
	bitmap.resize(svgDimension.width*factor, svgDimension.height*factor);
	bitmap.fmt = pix::INT_ARGB;
	bitmap.linearPremul = true;
	// Raster with Cairo
	boost::shared_ptr<cairo_surface_t> surface(
	  cairo_image_surface_create_for_data(&bitmap.buf[0], CAIRO_FORMAT_ARGB32, bitmap.width, bitmap.height, bitmap.width * 4),
	  cairo_surface_destroy);
	boost::shared_ptr<cairo_t> dc(cairo_create(surface.get()), cairo_destroy);
	cairo_scale(dc.get(), factor, factor);
	rsvg_handle_render_cairo(svgHandle.get(), dc.get());
	// Change byte order from BGRA to RGBA
	for (uint32_t *ptr = reinterpret_cast<uint32_t*>(&*bitmap.buf.begin()), *end = ptr + bitmap.buf.size() / 4; ptr < end; ++ptr) {
		uint8_t* pixel = reinterpret_cast<uint8_t*>(ptr);
		uint8_t r = pixel[2], g = pixel[1], b = pixel[0], a = pixel[3];
		pixel[0] = r; pixel[1] = g; pixel[2] = b; pixel[3] = a;
	}
	bitmap.fmt = pix::CHAR_RGBA;
	// Write to cache so that it can be loaded faster the next time
	fs::path cache_filename = cache::constructSVGCacheFileName(filename, factor);
	fs::create_directories(cache_filename.parent_path());
	writePNG(cache_filename, bitmap);
}
