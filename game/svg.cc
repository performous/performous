#include "svg.hh"

#include "cache.hh"
#include "configuration.hh"
#include "image.hh"
#include "log.hh"

#include <librsvg/rsvg.h>
#include <cstdint>
#include <iostream>

// Avoid deprecation messages with new versions since Ubuntu 12.10.
#if LIBRSVG_MAJOR_VERSION * 10000 + LIBRSVG_MINOR_VERSION * 100 + LIBRSVG_MICRO_VERSION < 23602
#include <librsvg/rsvg-cairo.h>
#endif

void loadSVG(Bitmap& bitmap, fs::path const& filename) {
	float factor = config["graphic/svg_lod"].f();
	// Try to load a cached PNG instead
	if (cache::loadSVG(bitmap, filename, factor)) return;
	SpdLogger::debug(LogSystem::IMAGE, "Loading SVG file, path={}.", filename);
	// Open the SVG file in librsvg
#if !GLIB_CHECK_VERSION(2, 36, 0)   // Avoid deprecation warnings
	g_type_init();
#endif
	GError* pError = nullptr;
	std::shared_ptr<RsvgHandle> svgHandle(rsvg_handle_new_from_file(filename.string().c_str(), &pError), g_object_unref);
	if (pError) {
		g_error_free(pError);
		throw std::runtime_error("Unable to load " + filename.string());
	}
#if LIBRSVG_MAJOR_VERSION >= 2 && LIBRSVG_MINOR_VERSION >= 52
	gdouble svg_width = 0.0, svg_height = 0.0;
	rsvg_handle_get_intrinsic_size_in_pixels(svgHandle.get(), &svg_width, &svg_height);
	bitmap.resize(static_cast<unsigned>(svg_width * factor + 0.5f), static_cast<unsigned>(svg_height * factor + 0.5f));
#else //functions are deprecated since 2.46
	// Get SVG dimensions
	RsvgDimensionData svgDimension;
	rsvg_handle_get_dimensions(svgHandle.get(), &svgDimension);
	// Prepare the pixel buffer
	float width = static_cast<float>(svgDimension.width)*factor;
	float height = static_cast<float>(svgDimension.height)*factor;
	bitmap.resize(static_cast<unsigned>(width), static_cast<unsigned>(height));
#endif
	bitmap.fmt = pix::Format::INT_ARGB;
	bitmap.linearPremul = true;
	// Raster with Cairo
	std::shared_ptr<cairo_surface_t> surface(
	  cairo_image_surface_create_for_data(&bitmap.buf[0], CAIRO_FORMAT_ARGB32, static_cast<int>(bitmap.width), static_cast<int>(bitmap.height), static_cast<int>(bitmap.width * 4)),
	  cairo_surface_destroy);
	std::shared_ptr<cairo_t> dc(cairo_create(surface.get()), cairo_destroy);
	cairo_scale(dc.get(), factor, factor);
#if LIBRSVG_MAJOR_VERSION >= 2 && LIBRSVG_MINOR_VERSION >= 52
	RsvgRectangle viewport = {0,0, svg_width, svg_height};
	rsvg_handle_render_document(svgHandle.get(),dc.get(), &viewport, NULL);
#else
	rsvg_handle_render_cairo(svgHandle.get(), dc.get());
#endif
	// Change byte order from BGRA to RGBA
	for (std::uint32_t *ptr = reinterpret_cast<std::uint32_t*>(bitmap.buf.data()), *end = ptr + bitmap.buf.size() / 4; ptr < end; ++ptr) {
		std::uint8_t* pixel = reinterpret_cast<std::uint8_t*>(ptr);
		std::uint8_t r = pixel[2], g = pixel[1], b = pixel[0], a = pixel[3];
		pixel[0] = r; pixel[1] = g; pixel[2] = b; pixel[3] = a;
	}
	bitmap.fmt = pix::Format::CHAR_RGBA;
	// Write to cache so that it can be loaded faster the next time
	fs::path cache_filename = cache::constructSVGCacheFileName(filename, factor);
	fs::create_directories(cache_filename.parent_path());
	writePNG(cache_filename, bitmap);

}
