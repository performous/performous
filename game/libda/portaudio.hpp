#pragma once

/**
 * @file portaudio.hpp OOP / RAII wrappers & utilities for PortAudio library.
 */

#include <boost/thread.hpp>
#include <portaudio.h>
#include <cstdlib>
#include <stdexcept>
#include <stdint.h>

#include "../unicode.hh"

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
		DeviceInfo(int id, std::string n = "", int i = 0, int o = 0): name(n), idx(id), in(i), out(o) {}
		std::string desc() {
			std::ostringstream oss;
			oss << name << " (";
			if (in) oss << in << " in";
			if (in && out) oss << ", ";
			if (out) oss << out << " out";
			return oss.str() + ")";
		}
		std::string name;
		int idx;
		int in, out;
	};

	typedef std::vector<DeviceInfo> DeviceInfos;

	/// List of useless legacy devices of PortAudio that we want to omit...
	static char const* g_ignored[] = { "front", "surround40", "surround41", "surround50", "surround51", "surround71", "iec958", "spdif", "dmix", NULL };

	struct AudioDevices {
		static int count() { return Pa_GetDeviceCount(); }
		/// Constructor gets the PA devices into a vector
		AudioDevices() {
			for (unsigned i = 0, end = Pa_GetDeviceCount(); i != end; ++i) {
				PaDeviceInfo const* info = Pa_GetDeviceInfo(i);
				if (!info) continue;
				std::string name = convertToUTF8(info->name);
				for (unsigned j = 0; g_ignored[j] && !name.empty(); ++j) {
					if (name.find(g_ignored[j]) != std::string::npos) name.clear();
				}
				if (!name.empty()) devices.push_back(DeviceInfo(i, name, info->maxInputChannels, info->maxOutputChannels));
			}
		}
		/// Get a printable dump of the devices
		std::string dump() {
			std::ostringstream oss;
			oss << "PortAudio devices:" << std::endl;
			for (unsigned i = 0; i < devices.size(); ++i)
				oss << "  " << i << "   " << devices[i].name << " (" << devices[i].in << " in, " << devices[i].out << " out)" << std::endl;
			oss << std::endl;
			return oss.str();
		}
		DeviceInfos devices;
	};

	struct Init {
		Init() { PORTAUDIO_CHECKED(Pa_Initialize, ()); }
		~Init() { Pa_Terminate(); }
	};

	struct Params {
		PaStreamParameters params;
		Params(PaStreamParameters const& init = PaStreamParameters()): params(init) {
			// Some useful defaults so that things just work
			channelCount(2).sampleFormat(paFloat32).suggestedLatency(0.05);
		}
		Params& channelCount(int val) { params.channelCount = val; return *this; }
		Params& device(PaDeviceIndex val) { params.device = val; return *this; }
		Params& sampleFormat(PaSampleFormat val) { params.sampleFormat = val; return *this; }
		Params& suggestedLatency(PaTime val) { params.suggestedLatency = val; return *this; }
		Params& hostApiSpecificStreamInfo(void* val) { params.hostApiSpecificStreamInfo = val; return *this; }
		operator PaStreamParameters const*() const { return &params; }
	};

	template <typename Functor> int functorCallback(void const* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {
		return (*static_cast<Functor*>(userData))(input, output, frameCount, timeInfo, statusFlags);
	}

	class Stream {
		PaStream* m_handle;
	public:
		Stream(
		  PaStreamParameters const* input,
		  PaStreamParameters const* output,
		  double sampleRate,
		  unsigned long framesPerBuffer = paFramesPerBufferUnspecified,
		  PaStreamFlags flags = paNoFlag,
		  PaStreamCallback* callback = NULL,
		  void* userData = NULL)
		{
			PORTAUDIO_CHECKED(Pa_OpenStream, (&m_handle, input, output, sampleRate, framesPerBuffer, flags, callback, userData));
		}
		template <typename Functor> Stream(
		  Functor& functor,
		  PaStreamParameters const* input,
		  PaStreamParameters const* output,
		  double sampleRate,
		  unsigned long framesPerBuffer = paFramesPerBufferUnspecified,
		  PaStreamFlags flags = paNoFlag)
		{
			PORTAUDIO_CHECKED(Pa_OpenStream, (&m_handle, input, output, sampleRate, framesPerBuffer, flags, functorCallback<Functor>, &functor));
		}
		~Stream() {
			// Give audio a little time to shutdown but then just quit
			boost::thread audiokiller(Pa_CloseStream, m_handle);
			if (!audiokiller.timed_join(boost::posix_time::milliseconds(5000))) {
				std::cout << "PortAudio BUG: Pa_CloseStream hung for more than five seconds. Exiting program." << std::endl;
				exit(1);
			}
		}
		operator PaStream*() { return m_handle; }
	};

}
