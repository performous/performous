#ifndef LIBDA_AUDIO_HPP_INCLUDED
#define LIBDA_AUDIO_HPP_INCLUDED

/**
@file audio.hpp LibDA public interface.

Link with libda when you use this.
**/

// Library version number, may be used to test whether a certain function is
// available. Incremented by one each time a new release is done.

#define LIBDA_VERSION 0

#include "sample.hpp"
#include <boost/function.hpp>
#include <cmath>
#include <cstddef>
#include <iosfwd>
#include <iterator>
#include <string>
#include <vector>
#include <stdint.h>

namespace da {

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

	class settings;
	class pcm_data;
	typedef boost::function<void (pcm_data& it, settings const&)> callback_t;

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

