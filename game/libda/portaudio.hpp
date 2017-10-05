#pragma once

/**
 * @file portaudio.hpp OOP / RAII wrappers & utilities for PortAudio library.
 */

#include <boost/thread.hpp>
#include <boost/regex.hpp>
#include <portaudio.h>
#include <cstdlib>
#include <stdexcept>
#include <stdint.h>

#include "../unicode.hh"
#include "../platform.hh"

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
		DeviceInfo(int id, std::string n = std::string(), int i = 0, int o = 0): name(n), flex(n), idx(id), in(i), out(o) {}
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
		int idx;
		int in, out;
	};
		typedef std::vector<DeviceInfo> DeviceInfos;
		struct AudioDevices {
		static int count() { return Pa_GetDeviceCount(); }
		static const PaHostApiTypeId AutoBackendType = PaHostApiTypeId(1337);
		static PaHostApiTypeId defaultBackEnd() {
			return PaHostApiTypeId(Platform::defaultBackEnd());
		}
		/// Constructor gets the PA devices into a vector
		AudioDevices(PaHostApiTypeId backend = AutoBackendType) {
		PaHostApiIndex backendIndex = Pa_HostApiTypeIdToHostApiIndex((backend == AutoBackendType ? defaultBackEnd() : backend));
		if (backendIndex == paHostApiNotFound) backendIndex = Pa_HostApiTypeIdToHostApiIndex(defaultBackEnd());
			for (unsigned i = 0, end = Pa_GetHostApiInfo(backendIndex)->deviceCount; i != end; ++i) {
				PaDeviceInfo const* info = Pa_GetDeviceInfo(Pa_HostApiDeviceIndexToDeviceIndex(backendIndex, i));
				if (!info) continue;
				std::string name = convertToUTF8(info->name);
				/// Omit some useless legacy devices of PortAudio/ALSA from our list
				for (auto const& dev: { "front", "surround40", "surround41", "surround50", "surround51", "surround71", "iec958", "spdif", "dmix" }) {
					if (name.find(dev) != std::string::npos) name.clear();
				}
				if (name.empty()) continue;  // No acceptable device found
				// Verify that the name is unique (haven't seen duplicate names occur though)
				std::string n = name;
				while (true) {
					int num = 1;
					for (auto& dev: devices) if (dev.name == n) goto rename;
					break;
				rename:
					std::ostringstream oss;
					oss << name << " #" << ++num;
					n = oss.str();
				};
				devices.push_back(DeviceInfo(i, name, info->maxInputChannels, info->maxOutputChannels));
			}
			for (auto& dev: devices) {
				// Array of regex - replacement pairs
				static char const* const replacements[][2] = {
					{ "\\(hw:\\d+,", "(hw:," },  // Remove ALSA device numbers
					{ " \\(.*\\)", "" },  // Remove the parenthesis part entirely
				};
				for (auto const& rep: replacements) {
					std::string flex = boost::regex_replace(dev.flex, boost::regex(rep[0]), rep[1]);
					if (flex == dev.flex) continue;  // Nothing changed
					// Verify that flex doesn't find any wrong devices
					bool fail = false;
					try {
						if (find(flex).idx != dev.idx) fail = true;
					} catch (...) {}  // Failure to find anything is success
					if (!fail) dev.flex = flex;
				}
			}
		}
		/// Get a printable dump of the devices
		std::string dump() const {
			std::ostringstream oss;
			for (auto const& d: devices) { oss << "    #" << d.idx << " " << d.desc() << std::endl; }
			return oss.str();
		}
		DeviceInfo const& find(std::string const& name) {
			// Try name search with full match
			for (auto const& dev: devices) { if (dev.name == name) { return dev;  } }
			// Try name search with partial/flexible match
			for (auto const& dev: devices) {
				if (dev.name.find(name) != std::string::npos) { return dev; }
				if (dev.flex.find(name) != std::string::npos) { return dev; }
			}
			throw std::runtime_error("No such device.");
		}
		DeviceInfos devices;
	};
	
	struct BackendInfo {
		BackendInfo(int id, PaHostApiTypeId type, std::string n = std::string(), int n_dev = 0): idx(id), name(n), type(type), devices(n_dev) {}
		int idx;
		std::string name;
		PaHostApiTypeId type;
		int devices;
		std::string desc () const {
		std::ostringstream oss;
		oss << "  #" << idx << ": " << name << " (" << devices << " devices):" << std::endl;
		oss << portaudio::AudioDevices(type).dump();
		return oss.str();
		}
	};

	typedef std::vector<BackendInfo> BackendInfos;
	
	struct AudioBackends {
	static int count() { return Pa_GetHostApiCount(); }
	AudioBackends () {
		if (count() == 0) throw std::runtime_error("No suitable audio backends found."); // Check specifically for 0 because it returns a negative error code if Pa is not initialized.
		for (unsigned i = 0, end = Pa_GetHostApiCount(); i != end; ++i) {
			PaHostApiInfo const* info = Pa_GetHostApiInfo(i);
			if (!info || info->deviceCount < 1) continue;
			/*
			Constant, unique identifier for each audio backend past alpha status.
				1 = DirectSound
				2 = MME
				3 = ASIO
				4 = SoundManager
				5 = CoreAudio
				7 = OSS
				8 = ALSA
				9 = AL
				10 = BeOS
				11 = WDMKS
				12 = JACK
				13 = WASAPI
				14 = AudioScienceHPI
				0 = Backend currently being developed.
			*/
			PaHostApiTypeId apiID = info->type;
			std::string name = convertToUTF8(info->name);
			backends.push_back(BackendInfo(i, apiID, name, info->deviceCount));		
		}
	};
		BackendInfos backends;
		
		std::string dump() const {
		std::ostringstream oss;
		oss << "audio/info: PortAudio backends:" << std::endl;
		for (auto const& b: backends) { oss << b.desc() << std::endl; }
			return oss.str();
		}
		std::list<std::string> getBackends() {
			std::set<std::string> bends;
			for (auto const& temp: backends) {
				bends.insert(temp.name);
			}
			return std::list<std::string>(bends.begin(),bends.end());
		}
	};

	struct Init {
		Init() { PORTAUDIO_CHECKED(Pa_Initialize, ()); }
		~Init() { Pa_Terminate(); }
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

	template <typename Functor> int functorCallback(void const* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {
		Functor* ptr = *reinterpret_cast<Functor**>(&userData);
		return (*ptr)(input, output, frameCount, timeInfo, statusFlags);
	}

	class Stream {
		PaStream* m_handle;
	public:
		/// Construct a stream as with Pa_OpenStream
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
		/// Construct stream using a C++ functor as callback
		template <typename Functor> Stream(
		  Functor& functor,
		  PaStreamParameters const* input,
		  PaStreamParameters const* output,
		  double sampleRate,
		  unsigned long framesPerBuffer = paFramesPerBufferUnspecified,
		  PaStreamFlags flags = paNoFlag
		): Stream(input, output, sampleRate, framesPerBuffer, flags, functorCallback<Functor>, (void*)(intptr_t)&functor) {}
		~Stream() {
			// Give audio a little time to shutdown but then just quit
			boost::thread audiokiller(Pa_CloseStream, m_handle);
			if (!audiokiller.timed_join(boost::posix_time::milliseconds(5000))) {
				std::cout << "PortAudio BUG: Pa_CloseStream hung for more than five seconds. Aborting." << std::endl;
				std::abort();  // Crash. Calling exit() is prone to hang.
			}
		}
		operator PaStream*() { return m_handle; }
	};

}
