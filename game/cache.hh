#pragma once

#include "configuration.hh"
#include "fs.hh"

#include <cstring>
#include <stdexcept>

namespace cache {

	/** Cache is for some reason invalid. (i.e. too old or not non existent) **/
	class invalid_cache_error : public std::runtime_error {
		/* This exception should always be caught so that the user never
		 * ever sees it. We shouldn't bother them with caching issues.
		 */
	  public:
		invalid_cache_error() : std::runtime_error("Invalid Cache. This error should never be seen.") {}
	};


	/** Builds the full path and file name for the SVG cache resource **/
	fs::path constructSVGCacheFileName(fs::path const& svgfilename, double factor);

	/** Given a path to an SVG the caching policy is returned **/
	inline bool cachableSVGResource(fs::path const& svgfilename) {
		// currently only theme files are cachable
		return isThemeResource(svgfilename);
	}

	/** Load an SVG from the cache, if loading fails invalid_cache_error is thrown **/
	template <typename T> void loadSVG(T& target, fs::path const& source_filename, double factor) {
		if(!cachableSVGResource(source_filename)) throw invalid_cache_error();

		fs::path const cache_filename = cache::constructSVGCacheFileName(source_filename, factor);
		if(!fs::exists(cache_filename)) throw invalid_cache_error();

		// SVG file is newer, so the cache is now invalid
		if(fs::last_write_time(source_filename) > fs::last_write_time(cache_filename))
			throw invalid_cache_error();

		try {
			loadPNG(target, cache_filename.string());
		} catch( ... ) { throw invalid_cache_error(); }
	}
}
