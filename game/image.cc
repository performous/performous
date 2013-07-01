#include "image.hh"

#include "cache.hh"
#include "configuration.hh"
#include <boost/filesystem/fstream.hpp>
#include <jpeglib.h>
#include <librsvg/rsvg.h>
#include <png.h>

using fs::ofstream;
using fs::ifstream;

// Avoid deprecation messages with new versions since Ubuntu 12.10.
#if LIBRSVG_MAJOR_VERSION * 10000 + LIBRSVG_MINOR_VERSION * 100 + LIBRSVG_MICRO_VERSION < 23602
#include <librsvg/rsvg-cairo.h>
#endif

namespace {
	void writePngHelper(png_structp pngPtr, png_bytep data, png_size_t length) {
		static_cast<std::ostream*>(png_get_io_ptr(pngPtr))->write((char*)data, length);
	}

	void readPngHelper(png_structp pngPtr, png_bytep data, png_size_t length) {
		static_cast<std::istream*>(png_get_io_ptr(pngPtr))->read((char*)data, length);
	}
	void loadPNG_internal(png_structp pngPtr, png_infop infoPtr, ifstream& file, Bitmap& bitmap, std::vector<png_bytep>& rows) {
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

	static void writePNG_internal(png_structp pngPtr, png_infop infoPtr, ofstream& file, unsigned w, unsigned h, pix::Format format, bool reverse, const unsigned char *data, std::vector<png_bytep>& rows) {
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

	struct my_jpeg_error_mgr {
		struct jpeg_error_mgr pub;	/* "public" fields */
		jmp_buf setjmp_buffer;	/* for return to caller */
	};

	typedef struct my_jpeg_error_mgr * my_jpeg_error_mgr_ptr;

	static inline void my_jpeg_error_exit(j_common_ptr cinfo) {
		my_jpeg_error_mgr_ptr myerr = (my_jpeg_error_mgr_ptr) cinfo->err;
		(*cinfo->err->output_message) (cinfo);
		longjmp(myerr->setjmp_buffer, 1);
	}

	#if JPEG_LIB_VERSION < 80
	// Implementation of jpeg_mem_src from
	// http://stackoverflow.com/questions/5280756/libjpeg-ver-6b-jpeg-stdio-src-vs-jpeg-mem-src 

	/* Read JPEG image from a memory segment */
	static void init_source(j_decompress_ptr /*cinfo*/) {}
	static boolean fill_input_buffer(j_decompress_ptr /*cinfo*/) {
		//ERREXIT(cinfo, JERR_INPUT_EMPTY);
		return true;
	}
	static void skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
		auto src = static_cast<jpeg_source_mgr*>(cinfo->src);
		if (num_bytes > 0) {
			src->next_input_byte += num_bytes;
			src->bytes_in_buffer -= num_bytes;
		}
	}
	static void term_source(j_decompress_ptr /*cinfo*/) {}
	static void jpeg_mem_src(j_decompress_ptr cinfo, void* buffer, long nbytes) {
		if (!cinfo->src) {   /* first time for this JPEG object? */
			cinfo->src = static_cast<jpeg_source_mgr*>(
			  (*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(jpeg_source_mgr))
			);
		}
		auto src = static_cast<jpeg_source_mgr*>(cinfo->src);
		src->init_source = init_source;
		src->fill_input_buffer = fill_input_buffer;
		src->skip_input_data = skip_input_data;
		src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
		src->term_source = term_source;
		src->bytes_in_buffer = nbytes;
		src->next_input_byte = static_cast<JOCTET*>(buffer);
	}
	#endif

}

void writePNG(fs::path const& filename, Image const& img) {
	std::vector<png_bytep> rows(img.h);
	ofstream file(filename, std::ios::binary);
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
	writePNG_internal(pngPtr, infoPtr, file, img.w, img.h, img.format, img.reverse, img.data.data(), rows);
}

void loadSVG(Bitmap& bitmap, fs::path const& filename) {
	double factor = config["graphic/svg_lod"].f();
	// Try to load a cached PNG instead
	if (cache::loadSVG(bitmap, filename, factor)) return;
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
	// Raster with Cairo
	boost::shared_ptr<cairo_surface_t> surface(
	  cairo_image_surface_create_for_data(&bitmap.buf[0], CAIRO_FORMAT_ARGB32, bitmap.width, bitmap.height, bitmap.width * 4),
	  cairo_surface_destroy);
	boost::shared_ptr<cairo_t> dc(cairo_create(surface.get()), cairo_destroy);
	cairo_scale(dc.get(), factor, factor);
	rsvg_handle_render_cairo(svgHandle.get(), dc.get());
	// Write to cache so that it can be loaded faster the next time
	fs::path cache_filename = cache::constructSVGCacheFileName(filename, factor);
	fs::create_directories(cache_filename.parent_path());
	cairo_surface_write_to_png(surface.get(), cache_filename.string().c_str());
}

void loadPNG(Bitmap& bitmap, fs::path const& filename) {
	ifstream file(filename, std::ios::binary);
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

void loadJPEG(Bitmap& bitmap, fs::path const& filename) {
	bitmap.fmt = pix::RGB;
	struct my_jpeg_error_mgr jerr;
	BinaryBuffer data = readFile(filename);
	jpeg_decompress_struct cinfo;
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_jpeg_error_exit;
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		throw std::runtime_error("Error in libjpeg when decoding " + filename.string());
	}
	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, data.data(), data.size());
	if (jpeg_read_header(&cinfo, true) != JPEG_HEADER_OK) throw std::runtime_error("Cannot read header of " + filename.string());
	jpeg_start_decompress(&cinfo);
	bitmap.resize(cinfo.output_width, cinfo.output_height);
	unsigned stride = (bitmap.width * 3 + 3) & ~3;  // Number of bytes per row (word-aligned)
	unsigned char* ptr = &bitmap.buf[0];
	while (cinfo.output_scanline < bitmap.height) {
		jpeg_read_scanlines(&cinfo, &ptr, 1);
		ptr += stride;
	}
	jpeg_destroy_decompress(&cinfo);
}

