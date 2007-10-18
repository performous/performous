#include "audio_dev.hpp"
#include <portaudio/portaudio.h>
#include <ostream>
#include <stdint.h>

namespace da {
	class pa18_record: public record::dev {
		static int c_callback(void* input, void*, unsigned long frames, PaTimestamp, void* userdata)
		{
			pa18_record& self = *static_cast<pa18_record*>(userdata);
			int16_t const* iptr = static_cast<int16_t const*>(input);
			try {
				std::vector<sample_t> buf(frames * self.s.channels());
				std::transform(iptr, iptr + buf.size(), buf.begin(), conv_from_s16);
				pcm_data data(&buf[0], frames, self.s.channels());
				self.s.callback()(data, self.s);
			} catch (std::exception& e) {
				self.s.debug(std::string("Exception from recording callback: ") + e.what());
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
			strm(pa18_record* rec) {
				// TODO: Use more settings from s
				PaError err = Pa_OpenStream(&handle, Pa_GetDefaultInputDeviceID(), rec->s.channels(), paInt16, NULL, paNoDevice, 0, paInt16, NULL, rec->s.rate, 50, 0, 0, rec->c_callback, rec);
				if (err != paNoError) throw std::runtime_error("Cannot open PortAudio audio stream " + rec->s.subdev() + ": " + Pa_GetErrorText(err));
			}
			~strm() { Pa_CloseStream(handle); }
		} stream;
	  public:
		pa18_record(settings& s): s(s), initialize(), stream(this) {
			PaError err = Pa_StartStream(stream.handle);
			if( err != paNoError ) throw std::runtime_error("Cannot start PortAudio audio stream " + s.subdev() + ": " + Pa_GetErrorText(err));
		}
	};
	namespace {
		record_plugin::reg<pa18_record> r(devinfo("pa18", "PortAudio v18 PCM capture. Settings are not used."));
	}
}
