#pragma once

#include "configuration.hh"
#include "fs.hh"
#include <cstring>
#include <stdexcept>

namespace cache {

	/** Builds the full path and file name for the SVG cache resource **/
	fs::path constructSVGCacheFileName(fs::path const& svgfilename, float factor);

	/** Load an SVG from the cache, if loading fails invalid_cache_error is thrown **/
	template <typename T> bool loadSVG(T& target, fs::path const& source_filename, float factor) {
		fs::path const cache_filename = cache::constructSVGCacheFileName(source_filename, factor);
		// Verify that a cached file exists and that it is more recent than the original SVG
		if (!fs::is_regular_file(cache_filename)) return false;
		if (fs::last_write_time(source_filename) > fs::last_write_time(cache_filename)) return false;
		// Try to load the cached file		
		try { 
			loadPNG(target, cache_filename.string()); 
		} catch( ... ) {
			return false;
		}
		return true;
	}
}

