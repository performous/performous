#pragma once

#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;

struct Bitmap;

void loadSVG(Bitmap& bitmap, fs::path const& filename);
