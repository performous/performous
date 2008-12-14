#ifndef FAAC_HH
#define FAAC_HH

#include <faac.h>
#include <algorithm>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace faac {

	namespace detail {
		template <typename Sample> float toFloat(Sample s);
		template <> float toFloat(float s) { return s; }
		template <> float toFloat(short s) { return s / 32767.0; }
	}
	
	class Enc {
	  public:
		Enc(unsigned long sampleRate, unsigned int numChannels, std::ostream& output):
		  m_handle(faacEncOpen(sampleRate, numChannels, &m_inputSamples, &sampleRate)),
		  m_outputBuffer(sampleRate), // HACK: Uses sampleRate as a tmp variable for output buffer size
		  m_output(output)
		{
			if (!m_handle) throw std::runtime_error("faacEncOpen returned NULL");
		}
		~Enc() {
			try { close(); } catch (...) {}
		}
		void close() {
			while (encode(NULL, 0)) {} // Flush the buffer
			faacEncClose(m_handle);
			m_handle = NULL;
		}
		operator faacEncHandle() { return m_handle; }
		template <typename InIt> void operator()(InIt begin, InIt end) {
			std::vector<float> inputBuffer;
			std::transform(begin, end, std::back_inserter(inputBuffer), detail::toFloat<typename std::iterator_traits<InIt>::value_type>);
			operator()(inputBuffer);
		}
		void operator()(std::vector<float> const& inputBuffer) {
			if (!m_handle) throw std::logic_error("Operating on closed faac::Enc");
			size_t pos = 0;
			for (size_t left; (left = inputBuffer.size() - pos) > 0;) {
				unsigned int num = std::min(m_inputSamples, left);
				encode(&inputBuffer[pos], num);
				pos += num;
			}
		}
	  private:
		int encode(float const* inputBuffer, unsigned int inputSamples) {
			int ret = faacEncEncode(m_handle, (int32_t*)inputBuffer, inputSamples, &m_outputBuffer[0], m_outputBuffer.size());
			if (ret < 0) throw std::runtime_error("faacEncEncode error");
			if (ret > 0) m_output.write(reinterpret_cast<char const*>(&m_outputBuffer[0]), ret);
			return ret;
		}
		unsigned long m_inputSamples;
		unsigned long m_maxOutputBytes;
		faacEncHandle m_handle;
		std::vector<unsigned char> m_outputBuffer;
		std::ostream& m_output;
	};
}

#endif

