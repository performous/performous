#include "cache.hh"
#include "fs.hh"
#include "util.hh"

#include <fmt/format.h>

namespace cache {
	fs::path constructSVGCacheFileName(fs::path const& svgfilename, float factor){
		std::string const lod = fmt::format("{:.2f}", factor);
		std::string const cache_basename = svgfilename.filename().string() + ".cache_" + lod + ".premul.png";
		// Windows drive name handling
		auto const fullpath = replace(svgfilename.parent_path().string(), ':', '_');

		return PathCache::getCacheDir() / "misc" / fs::path(fullpath).relative_path() / cache_basename;
	}
}
