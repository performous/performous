#include "portaudio.hh"
#include <libda/audio_dev.hpp>
#include <iostream>
#include <ostream>
#include <sstream>

namespace {

	struct Foo {
		portaudio::Init init;
		Foo() {
			std::clog << "PortAudio devices:\n";
			for (int i = 0, end = Pa_GetDeviceCount(); i != end; ++i) {
				PaDeviceInfo const* info = Pa_GetDeviceInfo(i);
				if (!info) continue;
				std::clog << "  pa19:" << i << "   " << info->name << " (" << info->maxInputChannels << " in, " << info->maxOutputChannels << " out)\n";
			}
			std::clog << std::endl;
		}
	} foo;

	using namespace da;
	class pa19_record: public record::dev {
		settings s;
		portaudio::Init init;
		portaudio::Stream stream;
	public:
		pa19_record(settings& s_orig):
		  s(s_orig),
		  stream(*this, portaudio::Params().channelCount(s.channels()).device(s.subdev(), true), NULL, s.rate())
		{
			PaError err = Pa_StartStream(stream);
			if( err != paNoError ) throw std::runtime_error("Cannot start PortAudio audio stream " + s.subdev() + ": " + Pa_GetErrorText(err));
			s_orig = s;
		}
		int operator()(void const* input, void*, unsigned long frames, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags)
		{
			try {
				pcm_data data((da::sample_t*)(input), frames, s.channels(), s.rate());
				s.callback()(data);
			} catch (std::exception& e) {
				s.debug(std::string("Exception from recording callback: ") + e.what());
			}
			return paContinue;
		}
	};
	plugin::simple<record_plugin, pa19_record> r(devinfo("pa19", "PortAudio v19 PCM capture. Device number as settings (otherwise PA default)."));

	class pa19_playback: public playback::dev {
		settings s;
		portaudio::Init init;
		portaudio::Stream stream;
	public:
		pa19_playback(settings& s_orig):
		  s(s_orig),
		  stream(*this, NULL, portaudio::Params().channelCount(s.channels()).device(s.subdev(), false), s.rate())
		{
			PaError err = Pa_StartStream(stream);
			if( err != paNoError ) throw std::runtime_error("Cannot start PortAudio audio stream " + s.subdev() + ": " + Pa_GetErrorText(err));
			s_orig = s;
		}
		int operator()(void const*, void* output, unsigned long frames, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags)
		{
			float* iptr = static_cast<da::sample_t*>(output);
			unsigned int samples = frames * s.channels();
			for (unsigned int i = 0; i < samples; i++) iptr[i] = 0;
			try {
				pcm_data data(iptr, frames, s.channels(), s.rate());
				s.callback()(data);
			} catch (std::exception& e) {
				s.debug(std::string("Exception from playback callback: ") + e.what());
			}
			return paContinue;
		}
	};
	plugin::simple<playback_plugin, pa19_playback> p(devinfo("pa19", "PortAudio v19 PCM playback. Settings are not used."));
}

