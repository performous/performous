#pragma once

/**
 * @file portaudio.hpp OOP / RAII wrappers & utilities for PortAudio library.
 */

#include "../log.hh"
#include "../platform.hh"
#include "../unicode.hh"

#include <fmt/format.h>
#include <portaudio.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <future>
#include <iostream>
#include <regex>
#include <set>
#include <stdexcept>

#define PORTAUDIO_CHECKED(func, args) portaudio::internal::check(func args, #func)

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
const PaHostApiTypeId V1337 = PaHostApiTypeId(1337);
#pragma GCC diagnostic pop
#else
#pragma warning(push)
#pragma warning(disable: 4244)  // C4244  "possible loss of data"
const PaHostApiTypeId V1337 = PaHostApiTypeId(1337);
#pragma warning(pop)
#endif

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

	/// The only purpose of this is throwing a _different_ error so we can report it as less severe to the user.
	class DeviceNotFoundError : public std::runtime_error {
		public:
			DeviceNotFoundError(std::string const& str) : std::runtime_error(str) {}
	}; 

	namespace internal {
		void inline check(PaError code, char const* func) { if (code != paNoError) throw Error(code, func); }
	}

	struct DeviceInfo {
		DeviceInfo(int id, std::string n = std::string(), int i = 0, int o = 0, int index = 0): name(n), flex(n), idx(id), in(i), out(o), index(index) {}
		std::string desc() const {
			std::string desc;
			fmt::format_to(std::back_inserter(desc), "{} (", name);
			if (in > 0) fmt::format_to(std::back_inserter(desc), "{} in", in);
			if (in > 0 && out > 0) desc.append(", ");
			if (out > 0) fmt::format_to(std::back_inserter(desc), "{} out", out);
			return desc.append(")");
		}
		std::string name;  ///< Full device name in UTF-8
		std::string flex;  ///< Modified name that is less specific but still unique (allow device numbers to change)
		int idx;
		int in, out;
		int index;
	};
	typedef std::vector<DeviceInfo> DeviceInfos;
	struct AudioDevices {
		static int count() { return Pa_GetDeviceCount(); }
		static const PaHostApiTypeId AutoBackendType {V1337};
		static PaHostApiTypeId defaultBackEnd() {
			return PaHostApiTypeId(Platform::defaultBackEnd());
		}
		/// Constructor gets the PA devices into a vector
		AudioDevices(PaHostApiTypeId backend = AutoBackendType) {
			PaHostApiIndex backendIndex = Pa_HostApiTypeIdToHostApiIndex((backend == AutoBackendType ? defaultBackEnd() : backend));
			if (backendIndex == paHostApiNotFound) backendIndex = Pa_HostApiTypeIdToHostApiIndex(defaultBackEnd());
			for (int i = 0, end = Pa_GetHostApiInfo(backendIndex)->deviceCount; i != end; ++i) {
				PaDeviceInfo const* info = Pa_GetDeviceInfo(Pa_HostApiDeviceIndexToDeviceIndex(backendIndex, i));
				if (!info) continue;
				std::string name = UnicodeUtil::convertToUTF8(info->name);
				/// Omit some useless legacy devices of PortAudio/ALSA from our list
				for (auto const& dev: { "front", "surround40", "surround41", "surround50", "surround51", "surround71", "iec958", "spdif", "dmix" }) {
					if (name.find(dev) != std::string::npos) name.clear();
				}
				if (name.empty()) continue;  // No acceptable device found
				// Verify that the name is unique (haven't seen duplicate names occur though)
				std::string n = name;
				while (true) {
					int num = 1;
					for (auto& dev: devices) if (dev.name == n) goto rename;  // FIXME: Possibly use std::find_if instead?
					break;
				rename:
					n = fmt::format("{} #{}", name, ++num);
				};
				devices.push_back(DeviceInfo(i, name, info->maxInputChannels, info->maxOutputChannels, Pa_HostApiDeviceIndexToDeviceIndex(backendIndex, i)));
			}
			for (auto& dev: devices) {
				// Array of regex - replacement pairs
				static char const* const replacements[][2] = {
					{ "\\(hw:\\d+,", "(hw:," },  // Remove ALSA device numbers
					{ " \\(.*\\)", "" },  // Remove the parenthesis part entirely
				};
				for (auto const& rep: replacements) {
					std::string flex = std::regex_replace(dev.flex, std::regex(rep[0]), rep[1]);
					if (flex == dev.flex) continue;  // Nothing changed
					// Verify that flex doesn't find any wrong devices
					bool fail = false;
					try {
						if (find(flex, false, 0).idx != dev.idx) fail = true;
					} catch (...) {}  // Failure to find anything is success
					if (!fail) dev.flex = flex;
				}
			}
		}
		/// Get a printable dump of the devices
		std::string dump() const {
			std::string dump;
			for (auto const& d: devices) {
				fmt::format_to(std::back_inserter(dump), "      {}#{}: {}\n", SpdLogger::newLineDec, d.idx, d.desc()); // 6 extra spaces, so it looks like a tree.
			}
			return dump;
		}
		DeviceInfo const& find(std::string const& name, bool output, int num) {
			if (name.empty()) { return findByChannels(output, num); }
			// Try name search with full match
			for (auto const& dev: devices) {
				if ((output ? dev.out : dev.in) < num) continue;
				if (dev.name == name) return dev;
			}
			// Try name search with partial/flexible match
			for (auto const& dev: devices) {
				if ((output ? dev.out : dev.in) < num) continue;
				if (dev.name.find(name) != std::string::npos) { return dev; }
				if (dev.flex.find(name) != std::string::npos) { return dev; }
			}
			throw DeviceNotFoundError{"No such device."};
		}
		DeviceInfo const& findByChannels(bool output, int num) {
			for (auto const& dev: devices) {
				int reqChannels = output ? dev.out : dev.in;
				if (reqChannels >= num) { return dev;  }
			}
			throw DeviceNotFoundError{"No such device."};
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
			std::string desc{fmt::format("#{}: {} ({} devices):\n", idx, name, devices)};
			return desc.append(portaudio::AudioDevices(type).dump());
		}
	};

	typedef std::vector<BackendInfo> BackendInfos;

	struct AudioBackends {
		static int count() { return Pa_GetHostApiCount(); }
		AudioBackends () {
			if (count() == 0) throw std::runtime_error("No suitable audio backends found."); // Check specifically for 0 because it returns a negative error code if Pa is not initialized.
			for (int i = 0, end = Pa_GetHostApiCount(); i != end; ++i) {
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
				std::string name = UnicodeUtil::convertToUTF8(info->name);
				backends.push_back(BackendInfo(i, apiID, name, info->deviceCount));
			}
		};
		BackendInfos backends;

		std::string dump() const {
			std::string dump{"PortAudio backends:\n"};
			for (auto const& b: backends) {
				fmt::format_to(std::back_inserter(dump), "{}{}", SpdLogger::newLineDec, b.desc());  // 13 spaces to account for the log timestamp plus the ::: separator.
			}
			return dump;
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

	template <typename Functor> static int functorCallback(void const* input, void* output, unsigned long frameCount,
                                                               const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData) {
		auto &callback = *reinterpret_cast<Functor*>(userData);
		return callback(reinterpret_cast<float const*>(input), reinterpret_cast<float*>(output), static_cast<std::int64_t>(frameCount));
	}

	class Stream {
		static void shutdownPaStream(PaStream *s) {
			try { PORTAUDIO_CHECKED(Pa_CloseStream, (s)); }
			catch(const std::exception &e) {
				SpdLogger::error(LogSystem::AUDIO, "Stream id={} failed to close. Exception={}", fmt::ptr(s), e.what());
			}
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
