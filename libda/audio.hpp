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
#include <boost/scoped_ptr.hpp>
#include <cmath>
#include <cstddef>
#include <iosfwd>
#include <iterator>
#include <string>
#include <vector>
#include <stdint.h>

namespace da {

	class init_impl;
	
	/** A RAII object for loading libda plugins. Create an object of this type
	* before you try to open audio devices. Reference counting is used internally
	* so that you can create multiple da::initialize objects.
	* Do not, however, destroy the object while audio devices are still in use!
	**/
	class initialize {
		init_impl* m_impl;
	  public:
		initialize();
		~initialize();
	};

	/** Provides various access iterators to a multich interleaved sample buffer. **/
	class pcm_data {
	  public:
		sample_t* m_buf;
		const std::size_t frames;
		const std::size_t channels;
		pcm_data(sample_t* buf, std::size_t _frames, std::size_t _channels): m_buf(buf), frames(_frames), channels(_channels) {}
		sample_t& operator()(std::size_t frame, std::size_t channel) { return m_buf[frame * channels + channel]; }
		sample_t& operator[](std::size_t idx) { return m_buf[idx]; }
		class iter_by_ch: public std::iterator<std::random_access_iterator_tag, sample_t> {
			sample_t* m_buf;
			std::size_t m_pos;
			std::size_t m_channels;
		  public:
			iter_by_ch(value_type* buf, std::size_t ch, std::size_t channels): m_buf(buf), m_pos(ch), m_channels(channels) {}
			value_type& operator*() { return m_buf[m_pos]; }
			iter_by_ch operator+(int rhs) { return iter_by_ch(m_buf, m_pos + m_channels * rhs, m_channels); }
			iter_by_ch& operator++() { m_pos += m_channels; return *this; }
			bool operator!=(iter_by_ch const& rhs) const { return m_pos != rhs.m_pos; }
			std::ptrdiff_t operator-(iter_by_ch const& rhs) const { return (m_pos - rhs.m_pos) / m_channels; }
			// TODO: more operators
		};
		iter_by_ch begin(std::size_t ch) { return iter_by_ch(m_buf, ch, channels); }
		iter_by_ch end(std::size_t ch) { return iter_by_ch(m_buf, ch, channels) + frames; }
	};

	class settings;
	class pcm_data;
	typedef boost::function<void (pcm_data& it, settings const&)> callback_t;

	struct settings {
		static const std::size_t low;
		static const std::size_t high;
		settings(std::string const& devstr = ""):
		  m_channels(high),
		  m_channels_near(true),
		  m_rate(high),
		  m_rate_near(true),
		  m_frames(low),
		  m_debug()
		{
			set_devstr(devstr);
		}
		void debug(std::string const& msg) { if (m_debug) *m_debug << msg << std::endl; }
		std::string const& device() const { return m_device; }
		std::string const& subdev() const { return m_subdev; }
		std::size_t const& frames() const { return m_frames; }
		std::size_t const& rate() const { return m_rate; }
		bool rate_near() const { return m_rate_near; }
		std::size_t const& channels() const { return m_channels; }
		bool channels_near() const { return m_channels_near; }
		callback_t callback() const { return m_callback; }
		settings& set_devstr(std::string const& devstr = "") {
	 		std::string::size_type pos = devstr.find(':');
			m_device = devstr.substr(0, pos);
			if (pos != std::string::npos) m_subdev = devstr.substr(pos + 1);
			return *this;
		}
		settings& set_device(std::string const& val) { m_device = val; return *this; }
		settings& set_subdev(std::string const& val) { m_subdev = val; return *this; }
		settings& set_callback(callback_t val) { m_callback = val; return *this; }
		settings& set_frames(std::size_t val) { m_frames = val; return *this; }
		settings& set_channels(std::size_t val, bool near = true) { m_channels = val; m_channels_near = near; return *this; }
		settings& set_rate(std::size_t val, bool near = true) { m_rate = val; m_rate_near = near; return *this; }
		settings& set_debug(std::ostream& val) { m_debug = &val; return *this; }
		settings& set_debug() { m_debug = NULL; return *this; }
	  private:
		callback_t m_callback;
		std::string m_device;
		std::string m_subdev;
		std::size_t m_channels;
		bool m_channels_near; // Allow non-exact channel counts
		std::size_t m_rate;
		bool m_rate_near; // Allow non-exact rates
		std::size_t m_frames; // Frames per block (requested)
		std::ostream* m_debug;
	};

	class devinfo {
		std::string m_name, m_desc;
	  public:
		devinfo(std::string const& _name, std::string const& _desc = ""): m_name(_name), m_desc(_desc) {}
		bool operator<(devinfo const& other) const { return name() < other.name(); }
		std::string const& name() const { return m_name; }
		std::string const& desc() const { return m_desc; }
		bool special() const { return !m_name.empty() && m_name[0] == '~'; }
	};
	
	class record {
	  public:
		typedef std::vector<devinfo> devlist_t;
		static devlist_t devices();
		record(settings& s);
		~record();
		class dev;
	  private:
		dev* m_handle;
	};

	class playback {
	  public:
		typedef std::vector<devinfo> devlist_t;
		static devlist_t devices();
		playback(settings& s);
		~playback();
		class dev;
	  private:
		dev* m_handle;
	};
}

#endif

