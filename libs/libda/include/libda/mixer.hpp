#ifndef LIBDA_MIXER_HPP_INCLUDED
#define LIBDA_MIXER_HPP_INCLUDED

/**
@file mixer.hpp LibDA mixer interface.

Link with libda when you use this.
**/

#include "audio.hpp"
#include <boost/scoped_ptr.hpp>
#include <algorithm>

namespace da {

	template <typename T> class shared_reference_wrapper {
	  public:
		explicit shared_reference_wrapper(boost::shared_ptr<T>& ptr): m_ptr(ptr) {}
		operator T&() const { return *m_ptr; }
	  private:
		boost::shared_ptr<T> m_ptr;
	};

	template <typename T> shared_reference_wrapper<T> shared_ref(boost::shared_ptr<T>& ptr) {
		return shared_reference_wrapper<T>(ptr);
	}

	class chain {
	  public:
		chain() {}
		chain(callback_t const& cb): streams(1, cb) {}
		typedef std::vector<callback_t> streams_t;
		streams_t streams;
		bool operator()(pcm_data& data, settings const& s) {
			streams_t::iterator wr = streams.begin();
			for (streams_t::iterator it = wr; it != streams.end(); ++it) {
				if ((*it)(data, s)) *wr++ = *it;
			}
			streams.erase(wr);
			return !streams.empty();
		}
	  private:
	};

	namespace {
		bool zero(pcm_data& data, settings const&) {
			std::fill(data.rawbuf, data.rawbuf + (data.channels * data.frames), 0.0f);
			return true;
		}
	}

	class buffer {
	  public:
		explicit buffer(size_t s): m_data(s) {}
		sample_t* begin() { return &m_data[0]; }
		sample_t* end() { return begin() + m_data.size(); }
	  private:
		std::vector<sample_t> m_data;
	};

	class stream {
	  public:
		bool operator()(pcm_data& data, settings const&) {
			sample_t* b = m_buffer->begin();
			sample_t* e = m_buffer->end();
			size_t size = std::min(data.frames * data.channels, e - b - m_bufferpos);
			for (size_t i = 0; i < size; ++i) {
				data.rawbuf[i] = b[m_bufferpos++];
			}
			return true;
		}
	  private:
		uint64_t m_streampos;
		boost::shared_ptr<buffer> m_buffer;
		size_t m_bufferpos;
	};

	class mixer {
	  public:
		mixer(settings s = settings()):
		  m_chain(zero),
		  m_settings(s.set_callback(m_chain)),
		  m_playback(m_settings)
		{}
	  private:
		chain m_chain;
		settings m_settings;
		playback m_playback;
	};
}

#endif

