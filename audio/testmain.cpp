#include "audio.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>

class FileOut {
	std::string filename;
	std::ofstream file;
	std::size_t frames;
	std::size_t channels;
	std::size_t rate;
  public:
	FileOut(char const* filename): filename(filename), file(filename, std::ios::binary), frames(), channels(), rate() {}
	~FileOut() {
		if (!channels) std::cout << "No data received: nothing written to " << filename << std::endl;
		else std::cout << frames << " frames (" << std::setprecision(3)
		  << double(frames) / rate << " s) written, playback using:\naplay -t raw -f FLOAT_LE -c"
		  << channels << " -r" << rate << " " << filename << std::endl;
	}
	/** Callback function **/
	void operator()(audio::pcm_data& data, audio::settings const& s) {
		channels = data.channels;
		rate = s.rate;
		// std::cout << data.frames << " frames received" << std::endl;
		for (std::size_t fr = 0; fr < data.frames; ++fr) {
			for (std::size_t ch = 0; ch < data.channels; ++ch) {
				audio::sample_t s = data(fr, ch);
				file.write(reinterpret_cast<char*>(&s), sizeof(s));
			}
		}
		frames += data.frames;
	}
};

int main(int argc, char** argv) {
	// Print the list of devices
	{
		std::vector<std::string> dev = audio::record::devices();
		std::cout << "Available audio devices: ";
		std::copy(dev.begin(), dev.end(), std::ostream_iterator<std::string>(std::cout, " "));
		std::cout << std::endl;
	}
	// Create output file
	FileOut fileout("testout.raw");
	// Construct settings
	audio::settings rs(argc > 1 ? argv[1] : "");
	rs.callback = boost::ref(fileout);
	rs.channels = 2;
	rs.rate = 48000;
	rs.frames = 1024;
	rs.debug = &std::cerr;
	{
		// Start recording (may modify settings)
		audio::record r(rs);
		std::cout << "# Device:            " << rs.device << ":" << rs.subdev << std::endl;
		std::cout << "# Sampling rate:     " << rs.rate << std::endl;
		std::cout << "# Channels:          " << rs.channels << std::endl;
		std::cout << "# Frames per block:  " << rs.frames << std::endl;
		std::cout << "PRESS ENTER TO STOP" << std::endl;
		std::cin.get();
		// Stop recording (r goes out of scope)
	}
	std::cout << "EXITING" << std::endl;
}

