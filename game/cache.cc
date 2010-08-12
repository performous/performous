#include "cache.hh"

#include "fs.hh"

#include <boost/format.hpp>

namespace cache {
	fs::path constructSVGCacheFileName(fs::path const& svgfilename, double factor){
		fs::path cache_filename;
		std::string const lod = (boost::format("%.2f") % factor).str();
		std::string const cache_basename = svgfilename.filename() + ".cache_" + lod + ".png";

		// presently only theme resources are cachable, "true ||" makes sure the compiler optimize out the conditional code
		if(true || isThemeResource(svgfilename)){
			std::string const theme_name = (config["game/theme"].s().empty() ? "default" : config["game/theme"].s());
			cache_filename = getCacheDir() / "themes" / theme_name / cache_basename;
		}else {
			// RFC: is this a sensible "fallback"?
			cache_filename = getCacheDir() / "misc" / cache_basename;
		}

		return cache_filename;
	}

}
