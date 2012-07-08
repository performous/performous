#pragma once

#include "cache.hh"
#include "configuration.hh"
#include "surface.hh"
#include "util.hh"
#include <jpeglib.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <png.h>
#include <fstream>

struct Image {
	std::vector<unsigned char> data;
	unsigned w, h;
	pix::Format format;
	bool reverse;
};

namespace {
	void readPngHelper(png_structp pngPtr, png_bytep data, png_size_t length) {
		static_cast<std::istream*>(png_get_io_ptr(pngPtr))->read((char*)data, length);
	}
	void writePngHelper(png_structp pngPtr, png_bytep data, png_size_t length) {
		static_cast<std::ostream*>(png_get_io_ptr(pngPtr))->write((char*)data, length);
	}
	void loadPNG_internal(png_structp pngPtr, png_infop infoPtr, std::ifstream& file, Bitmap& bitmap, std::vector<png_bytep>& rows) {
		if (setjmp(png_jmpbuf(pngPtr))) throw std::runtime_error("Reading PNG failed");
		png_set_read_fn(pngPtr,(png_voidp)&file, readPngHelper);
		png_read_info(pngPtr, infoPtr);
		png_set_expand(pngPtr);  // Expand everything to RGB(A)
		png_set_strip_16(pngPtr);  // Strip everything down to 8 bit/component
		png_set_gray_to_rgb(pngPtr);  // Convert even grayscale to RGB(A)
		png_set_filler(pngPtr, 0xFF, PNG_FILLER_AFTER); // Add alpha channel if it is missing
		bitmap.resize(png_get_image_width(pngPtr, infoPtr), png_get_image_height(pngPtr, infoPtr));
		rows.resize(bitmap.height);
		for (unsigned y = 0; y < bitmap.height; ++y) rows[y] = reinterpret_cast<png_bytep>(&bitmap.buf[y * bitmap.width * 4]);
		png_read_image(pngPtr, &rows[0]);
	}

	static void writePNG_internal(png_structp pngPtr, png_infop infoPtr, std::ofstream& file, unsigned w, unsigned h, pix::Format format, bool reverse, const unsigned char *data, std::vector<png_bytep>& rows) {
		// There must be no objects initialized within this function because longjmp will mess them up
		if (setjmp(png_jmpbuf(pngPtr))) throw std::runtime_error("Writing PNG failed");
		png_set_write_fn(pngPtr, &file, writePngHelper, NULL);
		unsigned char bpp = 3;
		switch(format) {
			case pix::RGB:
				png_set_IHDR(pngPtr, infoPtr, w, h, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
				bpp = 3;
				break;
			case pix::CHAR_RGBA:
				png_set_IHDR(pngPtr, infoPtr, w, h, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
				bpp = 4;
				break;
			default:
				// Note: INT_ARGB uses premultiplied alpha and cannot be supported by libpng
				throw std::logic_error("Unsupported pixel format in writePNG_internal");
		}
		png_write_info(pngPtr, infoPtr);
		unsigned stride = (w * bpp + 3) & ~3;  // Number of bytes per row (word-aligned)
		unsigned pos = reverse ? h * stride : -stride;
		for (unsigned y = 0; y < h; ++y) {
			if(reverse)
				pos -= stride;
			else
				pos += stride;
			rows[y] = (png_bytep)(&data[pos]);
		}
		png_write_image(pngPtr, &rows[0]);
		png_write_end(pngPtr, NULL);
	}

	static inline void writePNG(std::string const& filename, unsigned w, unsigned h, pix::Format format, bool reverse, const unsigned char *data) {
		std::vector<png_bytep> rows(h);
		std::ofstream file(filename.c_str(), std::ios::binary);
		png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!pngPtr) throw std::runtime_error("png_create_read_struct failed");
		png_infop infoPtr = NULL;
		struct Cleanup {
			png_structpp pngPP;
			png_infopp infoPP;
			Cleanup(png_structp& pngP, png_infop& infoP): pngPP(&pngP), infoPP(&infoP) {}
			~Cleanup() { png_destroy_write_struct(pngPP, infoPP); }
		} cleanup(pngPtr, infoPtr);
		infoPtr = png_create_info_struct(pngPtr);
		if (!infoPtr) throw std::runtime_error("png_create_info_struct failed");
		writePNG_internal(pngPtr, infoPtr, file, w, h, format, reverse, data, rows);
	}

	static inline void writePNG(std::string const& filename, Image const& img) {
		writePNG(filename, img.w, img.h, img.format, img.reverse, &img.data[0]);
	}

}

static inline void loadSVG(Bitmap& bitmap, std::string const& filename) {
	double factor = config["graphic/svg_lod"].f();
	// Try to load a cached PNG instead
	if (cache::loadSVG(bitmap, filename, factor)) return;
	// Open the SVG file in librsvg
	g_type_init();
	GError* pError = NULL;
	boost::shared_ptr<RsvgHandle> svgHandle(rsvg_handle_new_from_file(filename.c_str(), &pError), g_object_unref);
	if (pError) {
		g_error_free(pError);
		throw std::runtime_error("Unable to load " + filename);
	}
	// Get SVG dimensions
	RsvgDimensionData svgDimension;
	rsvg_handle_get_dimensions(svgHandle.get(), &svgDimension);
	// Prepare the pixel buffer
	bitmap.resize(svgDimension.width*factor, svgDimension.height*factor);
	bitmap.fmt = pix::INT_ARGB;
	// Raster with Cairo
	boost::shared_ptr<cairo_surface_t> surface(
	  cairo_image_surface_create_for_data(&bitmap.buf[0], CAIRO_FORMAT_ARGB32, bitmap.width, bitmap.height, bitmap.width * 4),
	  cairo_surface_destroy);
	boost::shared_ptr<cairo_t> dc(cairo_create(surface.get()), cairo_destroy);
	cairo_scale(dc.get(), factor, factor);
	rsvg_handle_render_cairo(svgHandle.get(), dc.get());
	// Write to cache so that it can be loaded faster the next time
	fs::path const cache_filename = cache::constructSVGCacheFileName(filename, factor);
	fs::create_directories(cache_filename.parent_path());
	cairo_surface_write_to_png(surface.get(), cache_filename.string().c_str());
}

static inline void loadPNG(Bitmap& bitmap, std::string const& filename) {
	// Raster with Cairo
	boost::shared_ptr<cairo_surface_t> surface(
	  cairo_image_surface_create_from_png(filename.c_str()),
	  cairo_surface_destroy);
	cairo_surface_flush(surface.get());
	unsigned char* buf = cairo_image_surface_get_data(surface.get());
	// Prepare the pixel buffer
	bitmap.resize(
	  cairo_image_surface_get_width(surface.get()),
	  cairo_image_surface_get_height(surface.get()));
	bitmap.fmt = pix::INT_ARGB;
	std::copy(buf, buf + bitmap.buf.size(), bitmap.buf.begin());
}

/*
static inline void loadPNG(Bitmap& bitmap, std::string const& filename) {
	std::ifstream file(filename.c_str(), std::ios::binary);
	png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!pngPtr) throw std::runtime_error("png_create_read_struct failed");
	png_infop infoPtr = NULL;
	struct Cleanup {
		png_structpp pngPP;
		png_infopp infoPP;
		Cleanup(png_structp& pngP, png_infop& infoP): pngPP(&pngP), infoPP(&infoP) {}
		~Cleanup() { png_destroy_read_struct(pngPP, infoPP, (png_infopp)NULL); }
	} cleanup(pngPtr, infoPtr);
	infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr) throw std::runtime_error("png_create_info_struct failed");
	std::vector<png_bytep> rows;
	loadPNG_internal(pngPtr, infoPtr, file, bitmap, rows);
}
*/
struct my_jpeg_error_mgr {
	struct jpeg_error_mgr pub;	/* "public" fields */
	jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_jpeg_error_mgr * my_jpeg_error_mgr_ptr;

static void my_jpeg_error_exit (j_common_ptr cinfo) {
	(void)my_jpeg_error_exit; // Silence a compile warning about this function being unused (in some source code modules)
	my_jpeg_error_mgr_ptr myerr = (my_jpeg_error_mgr_ptr) cinfo->err;
	(*cinfo->err->output_message) (cinfo);
	longjmp(myerr->setjmp_buffer, 1);
}

static inline void loadJPEG(Bitmap& bitmap, std::string const& filename) {
	bitmap.fmt = pix::RGB;
	struct my_jpeg_error_mgr jerr;
	FILE* infile = fopen(filename.c_str(), "rb");
	if (!infile) throw std::runtime_error("Cannot open " + filename);
	jpeg_decompress_struct cinfo;
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_jpeg_error_exit;
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		throw std::runtime_error("Error in libjpeg for file " + filename);
	}
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	if( jpeg_read_header(&cinfo, true) != JPEG_HEADER_OK) throw std::runtime_error("Cannot read header of " + filename);
	jpeg_start_decompress(&cinfo);
	bitmap.resize(cinfo.output_width, cinfo.output_height);
	unsigned stride = (bitmap.width * 3 + 3) & ~3;  // Number of bytes per row (word-aligned)
	unsigned char* ptr = &bitmap.buf[0];
	while (cinfo.output_scanline < bitmap.height) {
		jpeg_read_scanlines(&cinfo, &ptr, 1);
		ptr += stride;
	}
	jpeg_destroy_decompress(&cinfo);
	fclose(infile);
}

