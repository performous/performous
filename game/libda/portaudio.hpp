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
		std::string desc() {
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

	/// List of useless legacy devices of PortAudio that we want to omit...
	static char const* const g_ignored[] = { "front", "surround40", "surround41", "surround50", "surround51", "surround71", "iec958", "spdif", "dmix", NULL };

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
						if (find(flex) != dev.idx) fail = true;
					} catch (...) {}  // Failure to find anything is success
					if (!fail) dev.flex = flex;
				}
			}
		}
		/// Get a printable dump of the devices
		std::string dump() {
			std::ostringstream oss;
			oss << "PortAudio devices:" << std::endl;
			for (unsigned i = 0; i < devices.size(); ++i) oss << "  " << devices[i].desc() << std::endl;
			oss << std::endl;
			return oss.str();
		}
		int find(std::string const& name) {
			// Try name search with full match
			for (auto const& dev: devices) if (dev.name == name) return dev.idx;
			// Try name search with partial/flexible match
			for (auto const& dev: devices) {
				if (dev.name.find(name) != std::string::npos) return dev.idx;
				if (dev.flex.find(name) != std::string::npos) return dev.idx;
			}
			throw std::runtime_error("No such device.");
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
		Functor* ptr = *reinterpret_cast<Functor**>(&userData);
		return (*ptr)(input, output, frameCount, timeInfo, statusFlags);
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
			Functor* ptr = &functor;
			void* voidptr = *reinterpret_cast<void**>(&ptr);  // Trickery to avoid function pointer to data pointer cast (which is illegal in ISO C++)
			PORTAUDIO_CHECKED(Pa_OpenStream, (&m_handle, input, output, sampleRate, framesPerBuffer, flags, functorCallback<Functor>, voidptr));
		}
		~Stream() {
			// Give audio a little time to shutdown but then just quit
			boost::thread audiokiller(Pa_CloseStream, m_handle);
			if (!audiokiller.timed_join(boost::posix_time::milliseconds(5000))) {
				std::cout << "PortAudio BUG: Pa_CloseStream hung for more than five seconds. Exiting program." << std::endl;
				// Apparently some implementations put quick_exit in std:: and others in ::
				using namespace std;
				quick_exit(1);  // Do not kill atexit handlers that are also prone to hang...
			}
		}
		operator PaStream*() { return m_handle; }
	};

}
