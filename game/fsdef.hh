#pragma once

#ifdef USE_BOOST_FS
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

	#if BOOST_VERSION < 107400
	// create_directory was added in boost 1.74
	// see https://www.boost.org/users/history/version_1_74_0.html

	// in boost, copy_directory means create_directory
	// https://www.boost.org/doc/libs/1_66_0/libs/filesystem/doc/reference.html#copy_directory
	static inline void create_directory(const fs::path &to, const fs::path &from) {
		copy_directory(from, to);
	}

	#endif

#else
#include <filesystem>
namespace fs {

using namespace std::filesystem;

using std::ifstream;
using std::fstream;
using std::ofstream;

}

// Reimplment boost's absolute function with 2 parameters, according to its documentation:
// https://www.boost.org/doc/libs/1_51_0/libs/filesystem/doc/reference.html#absolute
static inline fs::path absolute(const fs::path& p, const fs::path& base) {
	if (p.has_root_directory()) {
		if (p.has_root_name())
			return p;
		else
			return fs::absolute(base).root_name() / p;
	} else {
		if (p.has_root_name())
			return p.root_name() / fs::absolute(base).root_directory() / fs::absolute(base).relative_path() / p.relative_path();
		else
			return fs::absolute(base) / p;
	}
}

#endif

