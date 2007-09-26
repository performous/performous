#include "audio_dev.hpp"
#include <portaudio.h>
#include <ostream>

namespace audio {
	class pa19_record: public record::dev {
		static reg_dev reg;
		static dev* create(settings& s) { return new pa19_record(s); }
		static int c_callback(const void* input, void*, unsigned long frames, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userdata)
		{
			pa19_record& self = *static_cast<pa19_record*>(userdata);
			int16_t const* iptr = static_cast<int16_t const*>(input);
			try {
				std::vector<sample_t> buf(frames * self.s.channels);
				std::transform(iptr, iptr + buf.size(), buf.begin(), conv_from_s16);
				pcm_data data(&buf[0], frames, self.s.channels);
				self.s.callback(data, self.s);
			} catch (std::exception& e) {
				if (self.s.debug) *self.s.debug << "Exception from recording callback: " << e.what() << std::endl;
			}
			return paContinue;
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
			strm(settings& s, PaCallback callback) {
				PaStreamParameters p;
				if (s.subdev.empty()) {
					p.device = Pa_GetDefaultInputDevice();
				} else {
					std::istringstream iss(s.subdev);
					iss >> p.device;
					if (!iss.eof() || p.device < 0 || p.device > Pa_GetDeviceCount() - 1) throw std::invalid_argument("Invalid PortAudio device number");
				}
				p.channelCount = s.channels;
				p.sampleFormat = paInt16;
				if (s.frames != settings::high) p.suggestedLatency = Pa_GetDeviceInfo(p.device)->defaultLowInputLatency;
				else p.suggestedLatency = Pa_GetDeviceInfo(p.device)->defaultHighInputLatency;
				p.hostApiSpecificStreamInfo = NULL;
				PaError err = Pa_OpenStream(&stream, &p, NULL, s.rate, 50, paClipOff, c_callback, this);
				if (err != paNoError) throw std::runtime_error("Cannot open PortAudio audio stream " + s.subdev + ": " + Pa_GetErrorText(err));
			}
			~strm() { Pa_CloseStream(handle); }
		} stream;
	  public:
		pa19_record(settings& s_orig): s(s_orig), initialize(), stream(s, c_callback) {
			PaError err = Pa_StartStream(stream.handle);
			if( err != paNoError ) throw std::runtime_error("Cannot start PortAudio audio stream " + s.subdev + ": " + Pa_GetErrorText(err));
			s_orig = s;
		}
	};

	pa19_record::reg_dev pa19_record::reg("pa", pa19_record::create);
}

