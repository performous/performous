#pragma once

#include "surface.hh"
#include "util.hh"

// Special kind of bitmap, used for writePNG only
struct Image {
	std::vector<unsigned char> data;
	unsigned w, h;
	pix::Format format;
	bool reverse;
};

void writePNG(fs::path const& filename, Image const& img);
void loadSVG(Bitmap& bitmap, fs::path const& filename);
void loadPNG(Bitmap& bitmap, fs::path const& filename);
void loadJPEG(Bitmap& bitmap, fs::path const& filename);

