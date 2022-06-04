#pragma once

/**
 * @file portaudio.hpp OOP / RAII wrappers & utilities for PortAudio library.
 */

#include "../unicode.hh"
#include <portaudio.h>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <regex>
#include <set>
#include <stdexcept>
#include <vector>

#define PORTAUDIO_CHECKED(func, args) portaudio::internal::check(func args, #func)

namespace portaudio {
	class Error: public std::runtime_error {
	public:
		Error(PaError code_, char const* func_):
		  runtime_error(std::string(func_) + " failed: " + Pa_GetErrorText(code_)),
		  m_code(code_),
		  m_func(func_)
		{}
		PaError code() const { return m_code; }
		std::string func() const { return m_func; }
	private:
		PaError m_code;
		char const* m_func;
	};

	namespace internal {
		void inline check(PaError code, char const* func) { if (code != paNoError) throw Error(code, func); }
	}

	struct DeviceInfo {
		DeviceInfo(int id, std::string n = std::string(), int i = 0, int o = 0, unsigned index = 0): name(n), flex(n), idx(id), in(i), out(o), index(index) {}
		std::string desc() const {
			std::ostringstream oss;
			oss << name << " (";
			if (in) oss << in << " in";
			if (in && out) oss << ", ";
			if (out) oss << out << " out";
			oss << ")";
			return oss.str();
		}
		std::string name;  ///< Full device name in UTF-8
		std::string flex;  ///< Modified name that is less specific but still unique (allow device numbers to change)
		unsigned idx;
		unsigned in, out;
		unsigned index;
	};
	typedef std::vector<DeviceInfo> DeviceInfos;

	struct AudioBackend {
		AudioBackend(const PaHostApiInfo &pa_info);
		~AudioBackend();
		AudioBackend(AudioBackend &&);
		AudioBackend &operator=(AudioBackend &&);

		DeviceInfo const& find(std::string const& name, bool output, unsigned num);

		DeviceInfos &getDevices();

                void dump() const;
	private:
		struct AudioDevices;
		std::reference_wrapper<const PaHostApiInfo> pa_info;
		std::unique_ptr<AudioDevices> audio_devices;
	};

	struct AudioBackendFactory {
		AudioBackendFactory();

		std::vector<std::string> getBackendsNames() const;

                AudioBackend makeBackend(std::string bakendName);
	private:
		struct Init;
		static Init init;
	};


	struct Params {
		PaStreamParameters params;
		Params(PaStreamParameters const& init = PaStreamParameters()): params(init) {
			// Some useful defaults so that things just work (channel count must be set by user anyway)
			if (params.channelCount == 0) sampleFormat(paFloat32).suggestedLatency(0.05);
		}
		Params& channelCount(int val) { params.channelCount = val; return *this; }
		Params& device(PaDeviceIndex val) { params.device = val; return *this; }
		Params& sampleFormat(PaSampleFormat val) { params.sampleFormat = val; return *this; }
		Params& suggestedLatency(PaTime val) { params.suggestedLatency = val; return *this; }
		Params& hostApiSpecificStreamInfo(void* val) { params.hostApiSpecificStreamInfo = val; return *this; }
		operator PaStreamParameters const*() const { return params.channelCount > 0 ? &params : nullptr; }
	};

	template <typename Functor> static int functorCallback(void const* input, void* output, unsigned long frameCount,
                                                               const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData) {
		auto &callback = *reinterpret_cast<Functor*>(userData);
		return callback(reinterpret_cast<float const*>(input), reinterpret_cast<float*>(output), frameCount);
	}

	class Stream {
		static void shutdownPaStream(PaStream *s) {
			try { PORTAUDIO_CHECKED(Pa_CloseStream, (s)); }
			catch(const std::exception &e) { std::cerr << "Failed to close stream " << s << " " << e.what() << '\n'; }
		}

		std::unique_ptr<PaStream, decltype(&shutdownPaStream)> m_handle{nullptr, &shutdownPaStream};
	public:
		/// Construct stream using a C++ functor as callback
		template <typename Functor> Stream(
		  Functor& functor,
		  PaStreamParameters const* input,
		  PaStreamParameters const* output,
		  double sampleRate,
		  unsigned long framesPerBuffer = paFramesPerBufferUnspecified,
		  PaStreamFlags flags = paNoFlag
		) {
			if (output != nullptr) {
				if (output->channelCount > 0) { flags = paPrimeOutputBuffersUsingStreamCallback; }
			}
			PaStream *new_stream;
			PORTAUDIO_CHECKED(Pa_OpenStream, (&new_stream, input, output, sampleRate, framesPerBuffer, flags, functorCallback<Functor>, (void*)(intptr_t)&functor));
			m_handle.reset(new_stream);

		}
		operator PaStream*() { return m_handle.get(); }
	};

}
