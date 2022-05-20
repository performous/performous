#include "fs.hh"
#include "image.hh"

#include <jpeglib.h>
#include <png.h>

#include <fstream>
#include <iostream>
#include <string>
#include <cstring>

namespace {
	void writePngHelper(png_structp pngPtr, png_bytep data, png_size_t length) {
		static_cast<std::ostream*>(png_get_io_ptr(pngPtr))->write((char*)data, static_cast<std::streamsize>(length));
	}

	void readPngHelper(png_structp pngPtr, png_bytep data, png_size_t length) {
		static_cast<std::istream*>(png_get_io_ptr(pngPtr))->read((char*)data, static_cast<std::streamsize>(length));
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

	static void writePNG_internal(png_structp pngPtr, png_infop infoPtr, std::ofstream& file, unsigned w, unsigned h, int colorType, std::vector<png_bytep>& rows) {
		// There must be no objects initialized within this function because longjmp will mess them up
		if (setjmp(png_jmpbuf(pngPtr))) throw std::runtime_error("Writing PNG failed");
		png_set_write_fn(pngPtr, &file, writePngHelper, nullptr);
		png_set_IHDR(pngPtr, infoPtr, w, h, 8, colorType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_write_info(pngPtr, infoPtr);
		png_write_image(pngPtr, &rows[0]);
		png_write_end(pngPtr, nullptr);
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

	#if JPEG_LIB_VERSION < 80 && !defined(MEM_SRCDST_SUPPORTED)
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

void writePNG(fs::path const& filename, Bitmap const& img, unsigned stride) {
	auto name = filename.string();
	// We use PNG in a non-standard way, with premultiplied alpha, signified by .premul.png extension.
	std::clog << "image/debug: Saving PNG: " + name << std::endl;
	std::vector<png_bytep> rows(img.height);
	// Determine color type and bytes per pixel
	unsigned char bpp;
	int colorType;
	switch (img.fmt) {
		case pix::Format::RGB: bpp = 3; colorType = PNG_COLOR_TYPE_RGB; break;
		case pix::Format::CHAR_RGBA: bpp = 4; colorType = PNG_COLOR_TYPE_RGBA; break;
		case pix::Format::BGR:
		case pix::Format::INT_ARGB:
		default:
			// Byte order would need to be changed for other formats and we don't currently need them...
			throw std::logic_error("Unsupported pixel format in writePNG_internal");
	}
	// Construct row pointers
	bool reverse = img.bottomFirst;
	if (stride == 0) stride = img.width * bpp;
	unsigned pos = reverse ? img.height * stride : -stride;
	for (unsigned y = 0; y < img.height; ++y) {
		pos += (reverse ? -stride : stride);
		rows[y] = (png_bytep)(&img.data()[pos]);
	}
	// Initialize libpng structures
	png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!pngPtr) throw std::runtime_error("png_create_read_struct failed");
	png_infop infoPtr = nullptr;
	struct Cleanup {
		png_structpp pngPP;
		png_infopp infoPP;
		Cleanup(png_structp& pngP, png_infop& infoP): pngPP(&pngP), infoPP(&infoP) {}
		~Cleanup() { png_destroy_write_struct(pngPP, infoPP); }
	} cleanup(pngPtr, infoPtr);
	infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr) throw std::runtime_error("png_create_info_struct failed");
	png_set_gAMA(pngPtr, infoPtr, img.linearPremul ? 1.0f : 2.2f);
	// Write file
	std::ofstream file(name, std::ios::binary);
	writePNG_internal(pngPtr, infoPtr, file, img.width, img.height, colorType, rows);
}

void loadPNG(Bitmap& bitmap, fs::path const& filename) {
	std::clog << "image/debug: Loading PNG: " + filename.string() << std::endl;
	// A hack to assume linear premultiplied data if file extension is .premul.png (used for cached SVGs)
	if (filename.stem().extension() == "premul") bitmap.linearPremul = true;
	std::ifstream file(filename.string(), std::ios::binary);
	png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!pngPtr) throw std::runtime_error("png_create_read_struct failed");
	png_infop infoPtr = nullptr;
	struct Cleanup {
		png_structpp pngPP;
		png_infopp infoPP;
		Cleanup(png_structp& pngP, png_infop& infoP): pngPP(&pngP), infoPP(&infoP) {}
		~Cleanup() { png_destroy_read_struct(pngPP, infoPP, (png_infopp)nullptr); }
	} cleanup(pngPtr, infoPtr);
	infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr) throw std::runtime_error("png_create_info_struct failed");
	std::vector<png_bytep> rows;
	loadPNG_internal(pngPtr, infoPtr, file, bitmap, rows);
}

void loadJPEG(Bitmap& bitmap, fs::path const& filename) {
	std::clog << "image/debug: Loading JPEG: " + filename.string() << std::endl;
	bitmap.fmt = pix::Format::RGB;
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
	if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) throw std::runtime_error("Cannot read header of " + filename.string());
	jpeg_start_decompress(&cinfo);
	bitmap.resize(cinfo.output_width, cinfo.output_height);
	unsigned stride = (bitmap.width * 3 + 3) & ~3u;  // Number of bytes per row (word-aligned)
	unsigned char* ptr = &bitmap.buf[0];
	while (cinfo.output_scanline < bitmap.height) {
		jpeg_read_scanlines(&cinfo, &ptr, 1);
		ptr += stride;
	}
	jpeg_destroy_decompress(&cinfo);
}

void Bitmap::crop(const unsigned width, const unsigned height, const unsigned x, const unsigned y) {
	if (ptr) throw std::logic_error("Cannot Bitmap::crop foreign pointers.");
	if (x + width > this->width || y+ height > this->height)
		throw std::logic_error("Cannot crop to a size bigger then source image.");

	unsigned char bpp;
	switch (fmt) {
	case pix::Format::INT_ARGB: bpp = 4; break; // Correct?
	case pix::Format::BGR: bpp = 3; break;
	case pix::Format::RGB: bpp = 3; break;
	case pix::Format::CHAR_RGBA: bpp = 4; break;
	default: throw std::logic_error("Unsupported picture format.");
	}

	unsigned newpos = 0;
	for (unsigned row = y; row < y + height; row++) {
		for (unsigned col = x; col < x + width; col++) {
			for (unsigned char subp = 0; subp < bpp; subp++) {
				unsigned oldpos = (row * this->width + col) * bpp + subp;
				if (oldpos != newpos) {
					buf[newpos] = buf[oldpos];
				}
				newpos++;
			}
		}
	}
	this->width = width;
	this->height = height;
}

void Bitmap::copyFromCairo(cairo_surface_t* surface) {
	unsigned width = static_cast<unsigned>(cairo_image_surface_get_width(surface));
	unsigned height = static_cast<unsigned>(cairo_image_surface_get_height(surface));
	resize(width, height);
	std::memcpy(&buf[0], cairo_image_surface_get_data(surface), buf.size());
}
