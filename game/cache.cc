#include "cache.hh"
#include "fs.hh"

#include <boost/format.hpp>
#include <algorithm>
#include <boost/algorithm/string/classification.hpp>

namespace cache {
	fs::path constructSVGCacheFileName(fs::path const& svgfilename, double factor){
		fs::path cache_filename;
		std::string const lod = (boost::format("%.2f") % factor).str();
		std::string const cache_basename = svgfilename.filename() + ".cache_" + lod + ".png";

		if (isThemeResource(svgfilename)) {
			std::string const theme_name = (config["game/theme"].s().empty() ? "default" : config["game/theme"].s());
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
