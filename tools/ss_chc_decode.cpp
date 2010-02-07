#include <cstdlib>
#include "chc_decode.hh"
#include <string>
#include <iostream>
#include <fstream>

int main(int argc, char ** argv) {
	if( argc != 6 ) {
		std::cout << "Usage: " << argv[0] << " chc_file key0 key1 key2 key4" << std::endl;
		return EXIT_FAILURE;
	}
	std::string key[4] = {argv[2], argv[3], argv[4], argv[5]};

	std::ifstream chc_file;
	chc_file.open(argv[1], std::ios_base::binary );

	// Get the file size
        chc_file.seekg (0, std::ios::end);
        unsigned int fileSize = chc_file.tellg();
        chc_file.seekg (0, std::ios::beg);

        // Reading inputfile
	char *buffer = new char[fileSize];
        std::cout << "Reading input file \"" << argv[1] << "\" (" << fileSize << " Bytes)... " << std::endl;
        chc_file.read(&buffer[0], fileSize);
	chc_file.close();

	ChcDecode chc_decoder;
	chc_decoder.load(key);
	std::string xmlMelody = chc_decoder.getMelody(buffer, fileSize, 31318);

	std::cout << xmlMelody << std::endl;

	delete[] buffer;

	return EXIT_SUCCESS;
}
