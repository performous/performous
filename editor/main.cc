#include "pitch.hh"
#include "ffmpeg.hh"
#include "notes.hh"
//#include <boost/math/special_functions/fpclassify.hpp>

int main(int argc, char **argv) {

	if (argc < 2)
	{
		std::cerr << "Not enough arguments" << std::endl;
		std::cerr << "Use " << argv[0] << " audiofile" << std::endl;

		return 1;
	}

	// Load audio
	FFmpeg mpeg(false, true, argv[1], 48000);
	Analyzer analyzer(48000, 1500);
	MusicalScale scale;

	// wait for ffmpeg to be ready
	// TODO: fix this it is probably not enough
	while(isinf(mpeg.duration()) ) ;

	// this should majorate the song duration
	int64_t duration = (mpeg.duration() + 0.5) * 48000 * 2;

	unsigned int i = 0;
	int64_t position = 0;
	std::vector<da::sample_t> sample(1500*2);
	da::pcm_data data(&sample[0], 1500, 2, 48000);
	while( mpeg.audioQueue(data, position) ) {
		std::cout << i << ": " << position << " " << double(position) / double(48000*2)<< std::endl;

		analyzer.input(data.begin(0), data.end(0));
		analyzer.process();

		Tone const* tone = analyzer.findTone();
		double freq = (tone ? tone->freq : 0.0);

		if( freq != 0.0 ) {
			std::cout << "  Found: " << freq << std::endl;
			Analyzer::tones_t tones = analyzer.getTones();
			for (Analyzer::tones_t::const_iterator t = tones.begin(); t != tones.end(); ++t) {
				if (t->age < Tone::MINAGE) continue;
				std::cout << "  " << t->freq << std::endl;
				int note = scale.getNoteId(t->freq);
				if (note < 0) continue;
				// Here I could spot X=position, Y=note, Z=t->db
			}
		} else {
			std::cout << "  No tone here" << std::endl;
		}
		i++;
	}

	std::cout << i << ": " << duration << " " << mpeg.duration() << std::endl;
	return EXIT_SUCCESS;
}
