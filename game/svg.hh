#pragma once

#include "fs.hh"

struct Bitmap;

void loadSVG(Bitmap& bitmap, fs::path const& filename);
