#pragma once

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
	void loadPNG_internal(png_structp pngPtr, png_infop infoPtr, std::ifstream& file, std::vector<unsigned char>& image, std::vector<png_bytep>& rows, unsigned& w, unsigned& h) {
		if (setjmp(png_jmpbuf(pngPtr))) throw std::runtime_error("Reading PNG failed");
		png_set_read_fn(pngPtr,(voidp)&file, readPngHelper);
		png_read_info(pngPtr, infoPtr);
		png_set_expand(pngPtr);  // Expand everything to RGB(A)
		png_set_strip_16(pngPtr);  // Strip everything down to 8 bit/component
		png_set_gray_to_rgb(pngPtr);  // Convert even grayscale to RGB(A)
		png_set_filler(pngPtr, 0xFF, PNG_FILLER_AFTER); // Add alpha channel if it is missing
		w = png_get_image_width(pngPtr, infoPtr);
		h = png_get_image_height(pngPtr, infoPtr);
		image.resize(w * h * 4);
		rows.resize(h);
		for (unsigned y = 0; y < h; ++y) rows[y] = reinterpret_cast<png_bytep>(&image[y * w * 4]);
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

template <typename T> void loadSVG(T& target, std::string const& filename, fs::path const& cache_filename = "") {
	struct RSVGInit {
		RSVGInit() { rsvg_init(); }
		~RSVGInit() { rsvg_term(); }
	} rsvgInit;
	GError* pError = NULL;
	// Find SVG dimensions (in pixels)
	RsvgHandle* svgHandle = rsvg_handle_new_from_file(filename.c_str(), &pError);
	if (pError) {
		g_error_free(pError);
		throw std::runtime_error("Unable to load " + filename);
	}
	RsvgDimensionData svgDimension;
	rsvg_handle_get_dimensions (svgHandle, &svgDimension);
	rsvg_handle_free(svgHandle);
	double factor = config["graphic/svg_lod"].f();
	unsigned int w = nextPow2(svgDimension.width*factor);
	unsigned int h = nextPow2(svgDimension.height*factor);
	// Load and raster the SVG
	GdkPixbuf* pb = rsvg_pixbuf_from_file_at_size(filename.c_str(), w, h, &pError);
	if (pError) {
		g_error_free(pError);
		throw std::runtime_error("Unable to load " + filename);
	}
	target.load(w, h, pix::CHAR_RGBA, gdk_pixbuf_get_pixels(pb), float(svgDimension.width)/svgDimension.height);
	gdk_pixbuf_unref(pb);

	if(!cache_filename.empty()) {
		// need to reload the svg to have the correct aspect ratio
		w = svgDimension.width * factor;
		h = svgDimension.height * factor;
		pb = rsvg_pixbuf_from_file_at_size(filename.c_str(), w, h, &pError);
		if (pError) {
			g_error_free(pError);
			throw std::runtime_error("Unable to load " + filename);
		}
		fs::create_directories(cache_filename.parent_path());
		writePNG(cache_filename.string(), w, h, pix::CHAR_RGBA, false, gdk_pixbuf_get_pixels(pb));
		gdk_pixbuf_unref(pb);
	}
}

template <typename T> void loadPNG(T& target, std::string const& filename) {
	std::vector<unsigned char> image;
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
	unsigned w, h;
	loadPNG_internal(pngPtr, infoPtr, file, image, rows, w, h);
	target.load(w, h, pix::CHAR_RGBA, &image[0], float(w)/h);
}

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

template <typename T> void loadJPEG(T& target, std::string const& filename) {
	std::vector<unsigned char> image;
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
	unsigned w = cinfo.output_width;
	unsigned h = cinfo.output_height;
	image.resize(w * h * 4);
	//unsigned stride = (w * 3 + 3) & ~3;  // Number of bytes per row (word-aligned)
	unsigned char* ptr = &image[0];
	while (cinfo.output_scanline < h) {
		jpeg_read_scanlines(&cinfo, &ptr, 1);
		ptr += (w * 3 + 3) & ~3;  // Rows need to be word aligned
	}
	jpeg_destroy_decompress(&cinfo);
	fclose(infile);
	target.load(w, h, pix::RGB, &image[0], float(w)/h);
}
