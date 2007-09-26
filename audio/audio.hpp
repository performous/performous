#ifndef AUDIO_HPP_INCLUDED
#define AUDIO_HPP_INCLUDED

#include <boost/function.hpp>
#include <cmath>
#include <cstddef>
#include <iosfwd>
#include <iterator>
#include <string>
#include <vector>
#include <stdint.h>

namespace audio {
	class settings;
	class pcm_data;

	// Can be changed to double, but there REALLY should be no need.
	typedef float sample_t;

	// The following conversions provide lossless conversions between floats
	// and integers. Negative minimum integer value produces sample_t value
	// slightly smaller than -1.0 but this is necessary in order to prevent
	// clipping in the float-to-int conversions while still staying lossless.
	// The roundf part is strictly not necessary, but it improves the rounding
	// error tolerance.
	const sample_t max_s16 = 32767.0;
	const sample_t max_s24 = 8388607.0;
	static inline sample_t conv_from_s16(int s) { return s / max_s16; }
	static inline sample_t conv_from_s24(int s) { return s / max_s24; }
	static inline int conv_to_s16(float s) { return int(roundf(s * max_s16)); }
	static inline int conv_to_s24(float s) { return int(roundf(s * max_s24)); }
	static inline int conv_to_s16(double s) { return int(round(s * max_s16)); }
	static inline int conv_to_s24(double s) { return int(round(s * max_s24)); }

	typedef boost::function<void (pcm_data& it, settings const&)> callback_t;

	/** Provides various access iterators to a multich interleaved sample buffer. **/
	class pcm_data {
		sample_t* buf;
	  public:
		const std::size_t frames;
		const std::size_t channels;
		pcm_data(sample_t* buf, std::size_t frames, std::size_t channels): buf(buf), frames(frames), channels(channels) {}
		sample_t& operator()(std::size_t frame, std::size_t channel) { return buf[frame * channels + channel]; }
		sample_t& operator[](std::size_t idx) { return buf[idx]; }
		class iter_by_ch: public std::iterator<std::random_access_iterator_tag, sample_t> {
			sample_t* buf;
			std::size_t pos;
			std::size_t channels;
		  public:
			iter_by_ch(value_type* buf, std::size_t ch, std::size_t channels): buf(buf), pos(ch), channels(channels) {}
			value_type& operator*() { return buf[pos]; }
			iter_by_ch operator+(int rhs) { return iter_by_ch(buf, pos + channels * rhs, channels); }
			iter_by_ch& operator++() { pos += channels; return *this; }
			bool operator!=(iter_by_ch const& rhs) const { return pos != rhs.pos; }
			std::ptrdiff_t operator-(iter_by_ch const& rhs) const { return (pos - rhs.pos) / channels; }
			// TODO: more operators
		};
		iter_by_ch begin(std::size_t ch) { return iter_by_ch(buf, ch, channels); }
		iter_by_ch end(std::size_t ch) { return iter_by_ch(buf, ch, channels) + frames; }
	};

	/*
	* Rather portable version that supports non-interleaved data as well...
	* Abandoned because it is too complex :/
	class pcm_data {
		std::vector<sample_t*> bufs;
		std::size_t step;
	  public:
		std::size_t frames;
		std::size_t channels;
		pcm_data(std::vector<sample_t*> const& bufs, std::size_t frames, std::size_t step):
		  bufs(bufs), step(step), frames(frames), channels(bufs.size()) {}
		sample_t& operator()(std::size_t frame, std::size_t channel) { return bufs[channel][frame * step]; }
		sample_t operator()(std::size_t frame, std::size_t channel) const { return bufs[channel][frame * step]; }
		// TODO: iterator classes
	};
	*/

	struct settings {
		callback_t callback;
		static const std::size_t low;
		static const std::size_t high;
		std::string device;
		std::string subdev;
		std::size_t channels;
		bool channels_near; // Allow non-exact channel counts
		std::size_t rate;
		bool rate_near; // Allow non-exact rates
		std::size_t frames; // Frames per block (requested)
		std::ostream* debug;
		settings(std::string const& devstr = ""):
		  channels(high),
		  channels_near(true),
		  rate(high),
		  rate_near(true),
		  frames(low),
		  debug()
		{
	 		std::string::size_type pos = devstr.find(':');
			device = devstr.substr(0, pos);
			if (pos != std::string::npos) subdev = devstr.substr(pos + 1);
		}
		settings& set_callback(callback_t val) { callback = val; return *this; }
		settings& set_channels(std::size_t val, bool near = true) { channels = val; channels_near = near; return *this; }
		settings& set_rate(std::size_t val, bool near = true) { rate = val; rate_near = near; return *this; }
		settings& set_debug(std::ostream& val) { debug = &val; return *this; }
		settings& set_debug() { debug = NULL; return *this; }
	};

	class record {
	  public:
		static std::vector<std::string> devices();
		record(settings& s);
		~record();
		class dev;
	  private:
		dev* handle;
	};

}

#endif

