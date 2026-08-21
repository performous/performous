#include "cache.hh"
#include "fs.hh"
#include "log.hh"
#include "util.hh"

#include <fmt/format.h>

namespace cache {
	fs::path constructSVGCacheFileName(fs::path const& svgfilename, float factor){
		std::string const lod = fmt::format("{:.2f}", factor);
		std::string const cache_basename = svgfilename.filename().string() + ".cache_" + lod + ".premul.png";
		// Windows drive name handling
		auto const fullpath = replace(svgfilename.parent_path().string(), ':', '_');
		SpdLogger::notice(LogSystem::FILESYSTEM, "svgfilename: {}", svgfilename.string());
		fs::path relativePath;
		if (Platform::currentOS() == Platform::HostOS::OS_LINUX) {
			relativePath = fs::relative(fs::path(fullpath), PathCache::getDataDir());
		}
		else {
			relativePath = fs::relative(fs::path(fullpath), PathCache::getDataDir());
		}
		std::string logmsg{fmt::format(
		"cache.cc:\n"
		"Found system paths:\n"
		"{8}base:          {0}\n"
		"{8}share:         {1}\n"
		"{8}locale:        {2}\n"
		"{8}sysConf:       {3}\n"
		"{8}home           {4}\n"
		"{8}config:        {5}\n"
		"{8}data:          {6}\n"
		"{8}cache:         {7}\n"
		"{8}SHARED_DATA_DIR: {9}",
		PathCache::getBaseDir().string(), PathCache::getShareDir().string(), PathCache::getLocaleDir().string(), PathCache::getSysConfigDir(), PathCache::getHomeDir().string(), PathCache::getConfigDir().string(), PathCache::getDataDir().string(), PathCache::getCacheDir().string(), SpdLogger::newLineDec, std::string{SHARED_DATA_DIR}
		)};
		SpdLogger::notice(LogSystem::FILESYSTEM, logmsg);
		return PathCache::getCacheDir() / "misc" / relativePath / cache_basename;
	}
}
