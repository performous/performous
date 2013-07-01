#include "adpcm.h"
#include <portaudio.h>

static int c_callback(void*, void* output, unsigned long framesPerBuffer, PaTimestamp, void* userdata) {
	AudioFifo* audioQueue = static_cast<AudioFifo*>(userdata);
	unsigned long size = 0;
	unsigned char channels = 2;
	std::vector<int16_t> buf;
	while(size < framesPerBuffer*channels) {
		size+=audioQueue->tryPop(buf, framesPerBuffer*channels-size);
	}
	int16_t* my_out = (int16_t*)output;
	for( unsigned int i = 0 ; i < size ; i++ ) {
		my_out[i] = buf[i];
	}
	return 0;
}

int main( int argc, char** argv) {
	std::ifstream m_f1;
	std::string in, out, fat;

	if( argc == 2 ) {
		fat = "none";
		in = argv[1];
	} else if( argc == 3 ) {
		fat = argv[1];
		in = argv[2];
	} else {
		std::cout << "Usage: " << argv[0] << " [fat_file] input_file" << std::endl;
		return EXIT_FAILURE;
	}

	Adpcm file(in,fat);
	PaError pa_err=Pa_Initialize();
	PaStream *stream;
	pa_err = Pa_OpenStream(&stream, paNoDevice, 0, paInt16, NULL, \
			Pa_GetDefaultOutputDeviceID(), \
			2, paInt16, NULL, 48000, 1234, 0, 0, \
			c_callback, &file.audioQueue);
	pa_err=Pa_StartStream(stream);
	while( !file.finished() || file.audioQueue.size() > 0) {
		sleep(1);

	}
	pa_err=Pa_CloseStream(stream);
	pa_err=Pa_Terminate();

	return EXIT_SUCCESS;
}
