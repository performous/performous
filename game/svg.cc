#include "svg.hh"

#include "cache.hh"
#include "configuration.hh"
#include "image.hh"

#include <iostream>
#include <gio/gio.h>
#include <librsvg/rsvg.h>

void loadSVG(Bitmap& bitmap, fs::path const& filename) {
	double factor = config["graphic/svg_lod"].f();
	// Try to load a cached PNG instead
	if (cache::loadSVG(bitmap, filename, factor)) return;
	std::clog << "image/debug: Loading SVG: " + filename.string() << std::endl;

	// Open the SVG file in librsvg
#if !GLIB_CHECK_VERSION(2, 36, 0)   // Avoid deprecation warnings
	g_type_init();
#endif

	GError* pError = nullptr;
    GFile *svgFile;
	svgFile = g_file_new_for_path (filename.string().c_str());	
	RsvgHandleFlags svgFlags = (RsvgHandleFlags) (RSVG_HANDLE_FLAG_UNLIMITED | RSVG_HANDLE_FLAG_KEEP_IMAGE_DATA);
	std::shared_ptr<RsvgHandle> svgHandle(rsvg_handle_new_from_gfile_sync(svgFile, svgFlags, NULL, &pError), g_object_unref);
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
    
    RsvgPositionData pos;
    rsvg_handle_get_position_sub (svgHandle.get(), &pos, nullptr);
	// Raster with Cairo
	std::shared_ptr<cairo_surface_t> surface(
	  cairo_image_surface_create(CAIRO_FORMAT_ARGB32, bitmap.width, bitmap.height),
	  cairo_surface_destroy);
	std::shared_ptr<cairo_t> dc(cairo_create(surface.get()), cairo_destroy);
	cairo_scale(dc.get(), factor, factor);
    cairo_translate (dc.get(), -pos.x, -pos.y);
    rsvg_handle_render_cairo_sub(svgHandle.get(), dc.get(), nullptr);
	// Copy rendered image to our buffer.
	if (bitmap.ptr) throw std::logic_error("Bitmap shouldn't point to a foreign object.");
	bitmap.copyFromCairo(surface.get());

	// Change byte order from BGRA to RGBA

	for (uint8_t *ptr = &*bitmap.buf.begin(), *end = ptr + bitmap.buf.size(); ptr < end; ptr += 4) {
		uint8_t* b = ptr;
		uint32_t pixel;
		std::memcpy(&pixel, b, sizeof(uint32_t));
		
		b[0] = (pixel & 0xff0000) >> 16;
		b[1] = (pixel & 0x00ff00) >> 8;
		b[2] = (pixel & 0x0000ff) >> 0;		
		b[3] = (pixel & 0xff000000) >> 24;
	}
	
	bitmap.fmt = pix::CHAR_RGBA;
	// Write to cache so that it can be loaded faster the next time
	fs::path cache_filename = cache::constructSVGCacheFileName(filename, factor);
	fs::create_directories(cache_filename.parent_path());
	writePNG(cache_filename, bitmap);
}
