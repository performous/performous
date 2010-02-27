#include <portaudio.h>
#include <cstdlib>
#include <stdexcept>
#include <stdint.h>

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
		void check(PaError code, char const* func) { if (code != paNoError) throw Error(code, func); }
	}

	struct Init {
		Init() { PORTAUDIO_CHECKED(Pa_Initialize, ()); }
		~Init() { Pa_Terminate(); }
	};

	struct Params {
		PaStreamParameters params;
		Params(PaStreamParameters const& init = PaStreamParameters()): params(init) {
			// Some useful defaults so that things just work
			channelCount(2).sampleFormat(paFloat32).suggestedLatency(0.02);
		}
		Params& channelCount(int val) { params.channelCount = val; return *this; }
		Params& device(PaDeviceIndex val) { params.device = val; return *this; }
		Params& device(std::string const& name, bool inputDevice) {
			int count = Pa_GetDeviceCount();
			int val = -1;
			if (name.empty()) val = inputDevice ? Pa_GetDefaultInputDevice() : Pa_GetDefaultOutputDevice();
			// Try numeric value
			if (val < 0) {
				std::istringstream iss(name);
				int tmp;
				if (iss >> tmp && iss.get() == EOF && tmp >= 0 && tmp < count) val = tmp;
			}
			// Try name matching
			if (val < 0) for (int i = 0; i != count; ++i) {
				PaDeviceInfo const* info = Pa_GetDeviceInfo(i);
				if (!info) continue;
				if (inputDevice && info->maxInputChannels == 0) continue;
				if (!inputDevice && info->maxOutputChannels == 0) continue;
				val = i;
				break;
			}
			// Error handling
			std::string dir = inputDevice ? "input" : "output";
			if (val < 0) throw std::runtime_error(name.empty() ? "No PortAudio default " + dir + " device found" : "No matching PortAudio " + dir + " device (" + name + ") found");
			PaDeviceInfo const* info = Pa_GetDeviceInfo(val);
			if (!info) throw std::runtime_error("The specified " + dir + " device (" + name + ") does not exist."); // FIXME: When does this happen?
			if ((inputDevice ? info->maxInputChannels : info->maxOutputChannels) == 0) throw std::runtime_error("The PortAudio " + dir + " device specified (" + name + ") has no " + dir + " channels");
			// Set the device
			return device(val);
		}
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
		~Stream() { Pa_CloseStream(m_handle); }
		operator PaStream*() { return m_handle; }
	};

}

