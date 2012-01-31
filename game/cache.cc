#include "cache.hh"
#include "fs.hh"

#include <boost/format.hpp>
#include <algorithm>
#include <boost/algorithm/string/classification.hpp>

namespace cache {
	fs::path constructSVGCacheFileName(fs::path const& svgfilename, double factor){
		fs::path cache_filename;
		std::string const lod = (boost::format("%.2f") % factor).str();
#if BOOST_FILESYSTEM_VERSION < 3
		std::string const cache_basename = svgfilename.filename() + ".cache_" + lod + ".png";
#else
		std::string const cache_basename = svgfilename.filename().string() + ".cache_" + lod + ".png";
#endif

		if (isThemeResource(svgfilename)) {
			std::string const theme_name = (config["game/theme"].getEnumName().empty() ? "default" : config["game/theme"].getEnumName());
			cache_filename = getCacheDir() / "themes" / theme_name / cache_basename;
		} else {
			// We use the full path under cache to avoid name collisions
			// with images other than theme files (mostly backgrounds).
			std::string fullpath = svgfilename.parent_path().string();
			// Windows drive name handling
			std::replace_if(fullpath.begin(), fullpath.end(), boost::is_any_of(":"), '_');
			cache_filename = getCacheDir() / "misc" / fullpath / cache_basename;
		}

		return cache_filename;
	}

}
