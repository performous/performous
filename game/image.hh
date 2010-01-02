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
};

namespace {
	void readPngHelper(png_structp pngPtr, png_bytep data, png_size_t length) {
		static_cast<std::istream*>(png_get_io_ptr(pngPtr))->read((char*)data, length);
	}
	void writePngHelper(png_structp pngPtr, png_bytep data, png_size_t length) {
		static_cast<std::ostream*>(png_get_io_ptr(pngPtr))->write((char*)data, length);
	}
}

template <typename T> void loadSVG(T& target, std::string const& filename) {
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
	// There must be no C++ objects after the setjmp line! (they won't get properly destructed)
	if (setjmp(png_jmpbuf(pngPtr))) throw std::runtime_error("Reading PNG failed");
	png_set_read_fn(pngPtr,(voidp)&file, readPngHelper);
	png_read_info(pngPtr, infoPtr);
	png_set_expand(pngPtr);
	png_set_strip_16(pngPtr);
	png_set_gray_to_rgb(pngPtr);
	unsigned w = png_get_image_width(pngPtr, infoPtr);
	unsigned h = png_get_image_height(pngPtr, infoPtr);
	unsigned channels = png_get_channels(pngPtr, infoPtr);
	if (channels == 1) channels = 3;  // Grayscale gets expanded to RGB
	if (channels == 2) channels = 4;  // Grayscale with alpha gets expanded to RGBA
	image.resize(w * h * 4);
	rows.resize(h);
	unsigned pos = 0;
	for (unsigned y = 0; y < h; ++y) {
		rows[y] = reinterpret_cast<png_bytep>(&image[pos]);
		pos += (w * channels + 3) & ~3;  // Rows need to be word aligned
	}
	png_read_image(pngPtr, &rows[0]);
	target.load(w, h, channels == 4 ? pix::CHAR_RGBA : pix::RGB, &image[0], float(w)/h);
}

template <typename T> void loadJPEG(T& target, std::string const& filename) {
	std::vector<unsigned char> image;
	FILE* infile = fopen(filename.c_str(), "rb");
	if (!infile) throw std::runtime_error("Cannot open " + filename);
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	jpeg_read_header(&cinfo, true);
	jpeg_start_decompress(&cinfo);
	unsigned w = cinfo.output_width;
	unsigned h = cinfo.output_height;
	image.resize(w * h * 4);
	unsigned stride = (w * 3 + 3) & ~3;  // Number of bytes per row (word-aligned)
	unsigned char* ptr = &image[0];
	while (cinfo.output_scanline < h) {
		jpeg_read_scanlines(&cinfo, &ptr, 1);
		ptr += (w * 3 + 3) & ~3;  // Rows need to be word aligned
	}
	jpeg_destroy_decompress(&cinfo);
	fclose(infile);
	target.load(w, h, pix::RGB, &image[0], float(w)/h);
}

static inline void writePNG(std::string const& filename, Image const& img) {
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
	std::vector<png_bytep> rows(img.h);
	// There must be no C++ objects after the setjmp line! (they won't get properly destructed)
	if (setjmp(png_jmpbuf(pngPtr))) throw std::runtime_error("Writing PNG failed");
	png_set_write_fn(pngPtr, &file, writePngHelper, NULL);
	png_set_IHDR(pngPtr, infoPtr, img.w, img.h, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);	
	png_write_info(pngPtr, infoPtr);
	unsigned stride = (img.w * 3 + 3) & ~3;  // Number of bytes per row (word-aligned)
	unsigned pos = img.h * stride;
	for (unsigned y = 0; y < img.h; ++y) {
		pos -= stride;
		rows[y] = (png_bytep)(&img.data[pos]);
	}
	png_write_image(pngPtr, &rows[0]);
}

