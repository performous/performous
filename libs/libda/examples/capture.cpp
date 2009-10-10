#include <boost/format.hpp>
#include <libda/audio.hpp>
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
	bool operator()(da::pcm_data& data) {
		channels = data.channels;
		rate = data.rate;
		// std::cout << data.frames << " frames received" << std::endl;
		for (std::size_t fr = 0; fr < data.frames; ++fr) {
			for (std::size_t ch = 0; ch < data.channels; ++ch) {
				da::sample_t s = data(fr, ch);
				file.write(reinterpret_cast<char*>(&s), sizeof(s));
			}
		}
		frames += data.frames;
		return true;
	}
};

int main(int argc, char** argv) {
	// Print the list of devices
	{
		std::vector<da::devinfo> l = da::record::devices();
		std::cout << "Capture devices available:" << std::endl;
		for (da::record::devlist_t::const_iterator it = l.begin(); it != l.end(); ++it) {
			std::cout << boost::format("  %1% %|10t|%2%\n") % it->name() % it->desc();
		}
	}
	// Create output file
	FileOut fileout("testout.raw");
	// Construct settings
	da::settings rs(argc > 1 ? argv[1] : "");
	rs.set_callback(boost::ref(fileout))
	  .set_channels(2)
	  .set_rate(48000)
	  .set_frames(1024)
	  .set_debug(std::cerr);
	da::initialize dainit; // Load da plugins
	try {
		// Start recording (may modify settings)
		da::record r(rs);
		std::cout << "# Device:            " << rs.device() << ":" << rs.subdev() << std::endl;
		std::cout << "# Sampling rate:     " << rs.rate() << std::endl;
		std::cout << "# Channels:          " << rs.channels() << std::endl;
		std::cout << "# Frames per block:  " << rs.frames() << std::endl;
		std::cout << "PRESS ENTER TO STOP" << std::endl;
		std::cin.get();
		// Stop recording (r goes out of scope)
	} catch (std::exception& e) {
		std::cout << "Unable to record: " << e.what() << std::endl;
	}
}

