#include "cache.hh"
#include "fs.hh"

#include <fmt/format.h>
#include <algorithm>

namespace cache {
	fs::path constructSVGCacheFileName(fs::path const& svgfilename, float factor){
		std::string const lod = fmt::format("{:.2f}", factor);
		std::string const cache_basename = svgfilename.filename().string() + ".cache_" + lod + ".premul.png";
		std::string fullpath = svgfilename.parent_path().string();
		// Windows drive name handling
		std::replace_if(fullpath.begin(), fullpath.end(), [](auto const c) { return c == ':'; }, '_');
		return getCacheDir() / "misc" / fs::path(fullpath).relative_path() / cache_basename;
	}

}
