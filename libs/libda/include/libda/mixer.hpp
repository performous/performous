#ifndef LIBDA_MIXER_HPP_INCLUDED
#define LIBDA_MIXER_HPP_INCLUDED

/**
@file mixer.hpp LibDA mixer interface.

Link with libda when you use this.
**/

#include "audio.hpp"
#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <algorithm>
#include <memory>

namespace da {

	template <typename T> class shared_reference_wrapper {
	  public:
		typedef T type;
		explicit shared_reference_wrapper(boost::shared_ptr<T> const& ptr): m_ptr(ptr) {}
		bool operator()(pcm_data data) { return get()(data); }
		operator T&() const { return *m_ptr; }
		T& get() const { return *m_ptr; }
		//T* get_pointer() const { return m_ptr; }
	  private:
		boost::shared_ptr<T> m_ptr;
	};

	template <typename T> shared_reference_wrapper<T> shared_ref(T* ptr) {
		return shared_reference_wrapper<T>(boost::shared_ptr<T>(ptr));
	}

	template <typename T> shared_reference_wrapper<T> shared_ref(boost::shared_ptr<T> const& ptr) {
		return shared_reference_wrapper<T>(ptr);
	}

	class chain: boost::noncopyable {
	  public:
		typedef std::vector<callback_t> streams_t;
		chain() {}
		chain(callback_t const& cb): streams(1, cb) {}
		void add(callback_t const& cb) {
			streams.push_back(cb);
		}
		/** Calls every scream in the chain and removes those that return false. **/
		bool operator()(pcm_data& data) {
			streams_t::iterator wr = streams.begin();
			for (streams_t::iterator it = wr; it != streams.end(); ++it) {
				if ((*it)(data)) *wr++ = *it;
			}
			if (wr != streams.end()) streams.erase(wr);
			return !streams.empty();
		}
		streams_t streams;
	  private:
	};

	namespace {
		bool zero(pcm_data& data) {
			std::fill(data.rawbuf, data.rawbuf + (data.channels * data.frames), 0.0f);
			return true;
		}
	}

	class buffer: boost::noncopyable {
	  public:
		explicit buffer(size_t s): m_data(s) {}
		sample_t* begin() { return &m_data[0]; }
		sample_t* end() { return begin() + m_data.size(); }
	  private:
		std::vector<sample_t> m_data;
	};

	class stream: boost::noncopyable {
	  public:
		stream(): m_pos() {}
		bool operator()(pcm_data& data) {
			sample_t* b = m_buffer->begin();
			sample_t* e = m_buffer->end();
			size_t size = std::min<size_t>(data.frames * data.channels, e - b - m_pos);
			for (size_t i = 0; i < size; ++i, ++m_pos) {
				if (m_pos < 0) continue;
				data.rawbuf[i] += b[m_pos];
			}
			return true;
		}
	  private:
		boost::shared_ptr<buffer> m_buffer;
		int64_t m_pos;
	};

	class fadeout: boost::noncopyable {
	  public:
		fadeout(callback_t stream, double time = 1.0): m_stream(stream), m_pos(), m_time(time) {}
		bool operator()(pcm_data& data) {
			bool ret = m_stream(data);
			size_t end = m_time * data.rate;
			size_t size = data.frames * data.channels;
			for (size_t i = 0; i < size; ++i, ++m_pos) {
				if (m_pos < 0) continue;
				sample_t level = size_t(m_pos) < end ? 1.0f - sample_t(m_pos) / end : 0.0f;
				data.rawbuf[i] *= level;
			}
			return ret && m_pos < int64_t(end);
		}
	  private:
		callback_t m_stream;
		int64_t m_pos;
		double m_time;
	};

	class scoped_lock: public boost::mutex::scoped_lock {
	  public:
		template <typename T> scoped_lock(T& obj): boost::mutex::scoped_lock(obj.m_mutex) {}
	};
	
	class mutex_stream: boost::noncopyable {
	  public:
		mutex_stream(callback_t const& stream): m_stream(stream) {}
		bool operator()(pcm_data& data) {
			scoped_lock l(*this);
			return m_stream(data);
		}
	  private:
		callback_t m_stream;
		boost::mutex m_mutex;
		friend class scoped_lock;
	};
	
	typedef std::auto_ptr<scoped_lock> lock_holder;
	
	class mixer {
	  public:
		mixer(): m_chain(zero), m_mutex(boost::ref(m_chain)) {}
		mixer(settings& s): m_chain(zero), m_mutex(boost::ref(m_chain)) { start(s); }
		void start(settings& s) { m_settings = s; start(); s = m_settings; }
		void start() {
			stop();
			m_settings.set_callback(boost::ref(m_mutex));
			m_playback.reset(new playback(m_settings));
		}
		void stop() { m_playback.reset(); }
		bool started() const { return m_playback; }
		void add(callback_t const& cb) {
			scoped_lock l(m_mutex);
			m_chain.add(cb);
		}
		void fade(double time = 1.0) {
			scoped_lock l(m_mutex);
			if (m_chain.streams.size() == 1) return;
			std::auto_ptr<chain> ch(new chain());
			ch->streams = chain::streams_t(m_chain.streams.begin() + 1, m_chain.streams.end());
			m_chain.streams.erase(m_chain.streams.begin() + 1, m_chain.streams.end());
			m_chain.add(shared_ref(new fadeout(shared_ref(ch.release()), time)));
		}
		settings get_settings() { return m_settings; }
		lock_holder lock() { return lock_holder(new scoped_lock(m_mutex)); }
	  private:
		chain m_chain;
		mutex_stream m_mutex;
		settings m_settings;
		boost::scoped_ptr<playback> m_playback;
	};
}

#endif

