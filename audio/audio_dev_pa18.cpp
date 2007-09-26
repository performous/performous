#include "audio_dev.hpp"
#include <portaudio/portaudio.h>
#include <ostream>
#include <stdint.h>

namespace audio {
	class pa18_record: public record::dev {
		static reg_dev reg;
		static dev* create(settings& s) { return new pa18_record(s); }
		static int c_callback(void* input, void*, unsigned long frames, PaTimestamp, void* userdata)
		{
			pa18_record& self = *static_cast<pa18_record*>(userdata);
			int16_t const* iptr = static_cast<int16_t const*>(input);
			try {
				std::vector<sample_t> buf(frames * self.s.channels);
				std::transform(iptr, iptr + buf.size(), buf.begin(), conv_from_s16);
				pcm_data data(&buf[0], frames, self.s.channels);
				self.s.callback(data, self.s);
			} catch (std::exception& e) {
				if (self.s.debug) *self.s.debug << "Exception from recording callback: " << e.what() << std::endl;
			}
			return 0;
		}
		settings s;
		struct init {
			init() {
				PaError err = Pa_Initialize();
				if( err != paNoError ) throw std::runtime_error(std::string("Cannot initialize PortAudio: ") + Pa_GetErrorText(err));
			}
			~init() { Pa_Terminate(); }
		} initialize;
		struct strm {
			PaStream* handle;
			strm(settings& s, PortAudioCallback callback) {
				// TODO: Use more settings from s
				PaError err = Pa_OpenStream(&handle, Pa_GetDefaultInputDeviceID(), 1, paInt16, NULL, paNoDevice, 0, paInt16, NULL, s.rate, 50, 0, 0, callback, this);
				if (err != paNoError) throw std::runtime_error("Cannot open PortAudio audio stream " + s.subdev + ": " + Pa_GetErrorText(err));
			}
			~strm() { Pa_CloseStream(handle); }
		} stream;
	  public:
		pa18_record(settings& s): s(s), initialize(), stream(s, c_callback) {
			PaError err = Pa_StartStream(stream.handle);
			if( err != paNoError ) throw std::runtime_error("Cannot start PortAudio audio stream " + s.subdev + ": " + Pa_GetErrorText(err));
		}
	};

	pa18_record::reg_dev pa18_record::reg("pa18", pa18_record::create);
}
