#ifndef USNG_ADPCM_H
#define USNG_ADPCM_H

#include <iostream>
#include <stdexcept>
#include <vector>

class Adpcm {
  public:
	/** Initialize the decoder.
	* Typical values for interleave are 0xB800 and 0xBB80. This information can
	* be extracted from music.mih
	**/
	Adpcm(unsigned int interleave_, unsigned int channels_ = 2): m_interleave(interleave_), headers(channels_) {}

	/** Decode 16 bytes of each channel, outputting 28 samples/ch. **/
	template <typename OutIt> OutIt decodeBlock(char const* data, OutIt pcm) {
		// Read headers
		for (unsigned ch = 0; ch < headers.size(); ++ch) headers[ch].parse(data + ch * m_interleave);
		for (unsigned n = 0; n < 28; ++n) {
			for (unsigned ch = 0; ch < headers.size(); ++ch) {
				Header& h = headers[ch];
				// Get a nibble and left-align it
				short sample = (data[2 + n / 2 + ch * m_interleave] << (n % 2 ? 8 : 12)) & 0xF000;
				int i_sample = sample;
				// Apply ADPCM exponent
				i_sample >>= h.shift;
				// Apply prediction
				i_sample += (h.pr1 * h.prev1 + h.pr2 * h.prev2 + 32) >> 6;
				if( i_sample > 32767 ) {
					sample = 32767;
				} else if( i_sample < -32768 ) {
					sample = -32768;
				} else {
					sample = i_sample;
				}
				// Store history for prediction
				h.prev2 = h.prev1;
				h.prev1 = sample;
				// Output sample
				*pcm++ = sample;
			}
		}
		return pcm;
	}

	unsigned int chunkFrames() const { return m_interleave / 16 * 28; }
	unsigned int chunkBytes() const { return m_interleave * 2; }
	void interleave(unsigned int _interleave) {m_interleave = _interleave; }

	/** Decode chunkBytes() bytes, outputting chunkFrames() samples/ch. **/
	template <typename OutIt> OutIt decodeChunk(char const* data, OutIt pcm) {
		for (unsigned pos = 0; pos < m_interleave; pos += 16) pcm = decodeBlock(data + pos, pcm);
		return pcm;
	}
  private:
	unsigned int m_interleave;
	struct Header {
		Header(): prev1(), prev2() {}
		int shift;
		int pr1;
		int pr2;
		char loopend;
		char loop;
		char loopstart;
		short prev1;
		short prev2;
		void parse(char const* data) {
			static const int f[5][2] = { { 0, 0 }, { 60, 0 }, { 115, -52 }, { 98, -55 }, { 122, -60 } };
			shift = data[0] & 0xF;
			unsigned mode = (data[0] >> 4) & 0xF;
			if (mode >= 5) throw std::runtime_error("Invalid mode");
			pr1 = f[mode][0];
			pr2 = f[mode][1];
			loopend = data[1] & 1;
			loop = (data[1] >> 1) & 1;
			loopstart = (data[1] >> 2) & 1;
		}
	};
	std::vector<Header> headers;
};

#endif
