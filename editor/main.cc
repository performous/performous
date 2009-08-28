#include "pitch.hh"
#include "ffmpeg.hh"

int main(int argc, char **argv) {
	// Load audio
	FFmpeg mpeg(false, true, argv[1], 48000);
	int64_t position = 0;

	da::sample_t *sample = new da::sample_t(2*4800);
	da::pcm_data data(sample, 1500, 2, 48000);

	while( mpeg.audioQueue(data, position) ) {
		std::cout << "Position: " << position << std::endl;
		position += (data.frames * data.channels);
	}

	std::cout << "Total: " << mpeg.audioQueue.duration() << std::endl;
	return EXIT_SUCCESS;
}
