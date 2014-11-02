#pragma once

#include "surface.hh"

#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;

// The total number of bytes per line (stride) may be specified. By default no padding at end of line is assumed.
void writePNG(fs::path const& filename, Bitmap const& bitmap, unsigned stride = 0);
void loadSVG(Bitmap& bitmap, fs::path const& filename);
void loadPNG(Bitmap& bitmap, fs::path const& filename);
void loadJPEG(Bitmap& bitmap, fs::path const& filename);

