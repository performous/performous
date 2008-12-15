#ifndef FAAC_HH
#define FAAC_HH

#include <faac.h>
#include <algorithm>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace faac {

	typedef int32_t Sample; // The internal sample type of FAAC (24 bit signed integer)

	namespace detail {
		template <typename SrcSample> Sample sampleConvert(SrcSample s);
		template <> Sample sampleConvert(double s) { return s * 8388607.0; }
		template <> Sample sampleConvert(float s) { return s * 8388607.0; }
		template <> Sample sampleConvert(short s) { return s << 8; }
		template <> Sample sampleConvert(int s) { return s; }
	}

	class Enc {
	  public:
		Enc(unsigned long sampleRate, unsigned int numChannels, std::ostream& output, unsigned int bitRatePerCh = 80000):
		  m_handle(faacEncOpen(sampleRate, numChannels, &m_inputSamples, &m_maxOutputBytes)),
		  m_outputBuffer(m_maxOutputBytes),
		  m_output(output)
		{
			if (!m_handle) throw std::runtime_error("faacEncOpen returned NULL");
			faacEncConfigurationPtr config = faacEncGetCurrentConfiguration(m_handle);
			config->mpegVersion = MPEG4;
			config->aacObjectType = LOW;
			config->allowMidside = 1;
			config->useLfe = 0;
			config->useTns = 0;
			config->bitRate = bitRatePerCh;
			config->bandWidth = sampleRate / 2;
			faacEncSetConfiguration(m_handle, config);
		}
		~Enc() {
			try { close(); } catch (...) {}
		}
		void close() {
			if (!m_handle) return;
			while (encode(NULL, 0)) {} // Flush the buffer
			faacEncClose(m_handle);
			m_handle = NULL;
		}
		operator faacEncHandle() { return m_handle; }
		template <typename InIt> Enc& operator()(InIt begin, InIt end) {
			std::vector<Sample> inputBuffer;
			std::transform(begin, end, std::back_inserter(inputBuffer), detail::sampleConvert<typename std::iterator_traits<InIt>::value_type>);
			return operator()(inputBuffer);
		}
		Enc& operator()(std::vector<Sample> const& inputBuffer) {
			if (!m_handle) throw std::logic_error("Operating on closed faac::Enc");
			size_t pos = 0;
			for (size_t left; (left = inputBuffer.size() - pos) > 0;) {
				unsigned int num = std::min<unsigned int>(m_inputSamples, left);
				encode(&inputBuffer[pos], num);
				pos += num;
			}
			return *this;
		}
	  private:
		int encode(Sample const* inputBuffer, unsigned int inputSamples) {
			int ret = faacEncEncode(m_handle, const_cast<Sample*>(inputBuffer), inputSamples, &m_outputBuffer[0], m_outputBuffer.size());
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

