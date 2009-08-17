#include <boost/lexical_cast.hpp>
#include <iostream>

#include "ss_cover.hh"

int main( int argc, char **argv) {
	if(argc != 4 ) {
		std::cout << "Usage: " << argv[0] << " [pak_file] [track_id] [output_image]" << std::endl;
		return EXIT_FAILURE;
	}

	SingstarCover c = SingstarCover(argv[1], boost::lexical_cast<unsigned int>(argv[2]));
	c.write(argv[3]);

	return EXIT_SUCCESS;
}
