#pragma once

#include <boost/filesystem/path.hpp>
#include <cairo/cairo.h>

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace fs = boost::filesystem;

namespace pix { enum Format {
	INT_ARGB,  // Cairo's pixel format (SVG, text): premultiplied linear RGB (BGRA byte order)
	CHAR_RGBA,  // libpng w/ alpha: non-premul sRGB (RGBA byte order)
	RGB,  // libpng w/o alpha, libjpeg, ffmpeg: sRGB (RGB byte order, no padding)
	BGR  // OpenCV/webcam: sRGB (BGR byte order, no padding)
}; }

struct Bitmap {
	std::vector<unsigned char> buf;  // Pixel data if owned by Bitmap
	unsigned char* ptr;  // Pixel data if owned by someone else
	unsigned width, height;
	float ar;  // Aspect ratio
	double timestamp;  // Used for video frames
	pix::Format fmt;
	bool linearPremul;  // Is the data linear RGB and premultiplied (as opposed to sRGB and non-premultiplied)
	bool bottomFirst;  // Upside-down (only used for taking screenshots)
	Bitmap(unsigned char* ptr = nullptr): ptr(ptr), width(), height(), ar(), timestamp(), fmt(pix::CHAR_RGBA), linearPremul(), bottomFirst() {}
	void resize(unsigned w, unsigned h) {
		if (!ptr) buf.resize(w * h * 4); else buf.clear();
		width = w;
		height = h;
		ar = float(w) / float(h);
	}
	void swap(Bitmap& b) {
		if (ptr || b.ptr) throw std::logic_error("Cannot Bitmap::swap foreign pointers.");
		buf.swap(b.buf);
		std::swap(width, b.width);
		std::swap(height, b.height);
		std::swap(ar, b.ar);
		std::swap(timestamp, b.timestamp);
		std::swap(fmt, b.fmt);
	}
	unsigned char const* data() const { return ptr ? ptr : &buf[0]; }
	unsigned char* data() { return ptr ? ptr : buf.data(); }
	void copyFromCairo(cairo_surface_t* surface);
	void crop(const unsigned width, const unsigned height, const unsigned x, const unsigned y);
};

// The total number of bytes per line (stride) may be specified. By default no padding at end of line is assumed.
void writePNG(fs::path const& filename, Bitmap const& bitmap, unsigned stride = 0);
void loadPNG(Bitmap& bitmap, fs::path const& filename);
void loadJPEG(Bitmap& bitmap, fs::path const& filename);

