#include "svg_cache.hh"
#include "fs.hh"
#include "log.hh"
#include "util.hh"

#include <fmt/format.h>

namespace svgCache {
	fs::path constructSVGCacheFileName(fs::path const& svgfilename, float factor){
		std::string const lod = fmt::format("{:.2f}", factor);
		std::string const cache_basename = svgfilename.filename().string() + ".cache_" + lod + ".premul.png";
		fs::path relativePath = fs::relative(svgfilename.parent_path(), PathCache::getShareDir());

		fs::path premulPath{PathCache::getCacheDir() / "misc" / relativePath / cache_basename};
		SpdLogger::debug(LogSystem::IMAGE, "SVG file={}, caching premultiplied PNG at path={}", svgfilename.string(), premulPath.string()); 

		return premulPath;
	}
}
