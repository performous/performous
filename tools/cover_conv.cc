#include "ss_cover.hh"

#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <cstdlib>
#ifdef __WIN32__
#  include <windows.h>
#endif

namespace fs = boost::filesystem;

int main( int argc, char **argv) {
	if(argc != 4 ) {
		std::cout << "Usage: " << argv[0] << " [-h|--help] pak_file track_id output_image" << std::endl;
		return EXIT_FAILURE;
	}

	SingstarCover c = SingstarCover(argv[1], boost::lexical_cast<unsigned int>(argv[2]));
	c.write(fs::path(argv[3]));

	return EXIT_SUCCESS;
}
