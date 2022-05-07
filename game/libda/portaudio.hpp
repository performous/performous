#pragma once

/**
 * @file portaudio.hpp OOP / RAII wrappers & utilities for PortAudio library.
 */

#include "../unicode.hh"
#include "../platform.hh"
#include <portaudio.h>
#include <cstdint>
#include <cstdlib>
#include <future>
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
	struct AudioDevices {
		static int count() { return Pa_GetDeviceCount(); }
		static PaHostApiTypeId defaultBackEnd() {
			return PaHostApiTypeId(Platform::defaultBackEnd());
		}
		/// Constructor gets the PA devices into a vector
		AudioDevices(PaHostApiTypeId backend = defaultBackEnd()) {
			auto backendIndex = Pa_HostApiTypeIdToHostApiIndex(backend);
			if (backendIndex == paHostApiNotFound) backendIndex = Pa_HostApiTypeIdToHostApiIndex(defaultBackEnd());
			for (unsigned i = 0, end = Pa_GetHostApiInfo(backendIndex)->deviceCount; i != end; ++i) {
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
					for (auto& dev: devices) if (dev.name == n) goto rename;
					break;
				rename:
					std::ostringstream oss;
					oss << name << " #" << ++num;
					n = oss.str();
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
			std::ostringstream oss;
			for (auto const& d: devices) { oss << "    #" << d.idx << " " << d.desc() << std::endl; }
			return oss.str();
		}
		DeviceInfo const& find(std::string const& name, bool output, unsigned num) {
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
			throw std::runtime_error("No such device.");
		}
		DeviceInfo const& findByChannels(bool output, unsigned num) {
			for (auto const& dev: devices) {
				unsigned reqChannels = output ? dev.out : dev.in;
				if (reqChannels >= num) { return dev;  }
			}
			throw std::runtime_error("No such device.");
		}
		DeviceInfos devices;
	};

	struct AudioBackendFactory {
		AudioBackendFactory();

		std::vector<std::string> getBackendsNames() const;
	private:
		void dump() const;

		std::vector<const PaHostApiInfo *> backends;

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
