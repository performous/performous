#pragma once

/**
 * @file mixer.hpp LibDA mixer interface, version 1.
 *
 * This appears to be too complex and will probably be removed in later release in
 * favor of something easier and faster. Actually it is not even used currently.
 */

#include "audio.hpp"
#include <algorithm>
#include <memory>
#include <mutex>

namespace da {

	template <typename T> class shared_reference_wrapper {
	  public:
		typedef T type;
		explicit shared_reference_wrapper(std::shared_ptr<T> const& ptr): m_ptr(ptr) {}
		bool operator()(pcm_data data) { return get()(data); }
		operator T&() const { return *m_ptr; }
		T& get() const { return *m_ptr; }
		//T* get_pointer() const { return m_ptr; }
	  private:
		std::shared_ptr<T> m_ptr;
	};

	template <typename T> shared_reference_wrapper<T> shared_ref(T* ptr) {
		return shared_reference_wrapper<T>(std::shared_ptr<T>(ptr));
	}

	template <typename T> shared_reference_wrapper<T> shared_ref(std::shared_ptr<T> const& ptr) {
		return shared_reference_wrapper<T>(ptr);
	}


	class chain {
	  public:
	  	chain(const chain&) = delete;
  		const chain& operator=(const chain&) = delete;
		typedef std::vector<callback_t> streams_t;
		void add(callback_t const& cb) { streams.push_back(cb); }
		void clear() { streams.clear(); }
		/** Calls every stream in the chain and removes those that return false. **/
		bool operator()(pcm_data& data) {
			streams_t::iterator wr = streams.begin();
			for (streams_t::iterator it = wr; it != streams.end(); ++it) {
				if ((*it)(data)) *wr++ = *it;
			}
			if (wr != streams.end()) streams.erase(wr, streams.end());
			return !streams.empty();
		}
		streams_t streams;
	  private:
	};

	namespace {
		bool voidOp(pcm_data const&) { return true; }
		bool zero(pcm_data& data) {
			std::fill(data.rawbuf, data.rawbuf + (data.channels * data.frames), 0.0f);
			return true;
		}
	}

	class accumulate {
	  public:
	  	accumulate(const accumulate&) = delete;
  		const accumulate& operator=(const accumulate&) = delete;
		typedef std::vector<callback_t> streams_t;
		void add(callback_t const& cb) { streams.push_back(cb); }
		void clear() { streams.clear(); }
		/** Calls every stream in the chain and removes those that return false, summing the results. **/
		bool operator()(pcm_data& data) {
			std::vector<sample_t> buffer(data.samples());
			pcm_data accumbuf(&buffer[0], data.frames, data.channels, data.rate);
			streams_t::iterator wr = streams.begin();
			for (streams_t::iterator it = wr; it != streams.end(); ++it) {
				if ((*it)(accumbuf)) *wr++ = *it;
				for (std::size_t i = 0; i < buffer.size(); ++i) {
					data.rawbuf[i] += buffer[i];
					buffer[i] = 0.0f;
				}
			}
			if (wr != streams.end()) streams.erase(wr, streams.end());
			return !streams.empty();
		}
		streams_t streams;
	  private:
	};

	class buffer {
	  public:
	  	buffer(const buffer&) = delete;
  		const buffer& operator=(const buffer&) = delete;
		explicit buffer(std::size_t s): m_data(s) {}
		sample_t* begin() { return &m_data[0]; }
		sample_t* end() { return begin() + m_data.size(); }
	  private:
		std::vector<sample_t> m_data;
	};

	class stream {
	  public:
	  	stream(const stream&) = delete;
  		const stream& operator=(const stream&) = delete;
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
		std::shared_ptr<buffer> m_buffer;
		std::int64_t m_pos;
	};

	class fadeinOp {
	  public:
	  	fadeinOp(const fadeinOp&) = delete;
  		const fadeinOp& operator=(const fadeinOp&) = delete;
		fadeinOp(double time = 1.0, std::int64_t pos = 0): m_pos(pos), m_time(time) {}
		bool operator()(pcm_data& data) {
			size_t end = m_time * data.rate;
			size_t size = data.frames * data.channels;
			if (m_pos > std::int64_t(end)) return 1.0f;
			for (size_t i = 0; i < size; ++i, ++m_pos) {
				if (m_pos < 0) data.rawbuf[i] = 0.0f;
				sample_t level = size_t(m_pos) < end ? sample_t(m_pos) / end : 1.0f;
				data.rawbuf[i] *= level;
			}
			return m_pos < std::int64_t(end);
		}
	  private:
		std::int64_t m_pos;
		double m_time;
	};

	class fadeoutOp {
	  public:
	  	fadeoutOp(const fadeoutOp&) = delete;
  		const fadeoutOp& operator=(const fadeoutOp&) = delete;
		fadeoutOp(callback_t cb, double time = 1.0, std::int64_t pos = 0): m_stream(cb), m_pos(pos), m_time(time) {}
		bool operator()(pcm_data& data) {
			bool ret = m_stream(data);
			size_t end = m_time * data.rate;
			size_t size = data.frames * data.channels;
			for (size_t i = 0; i < size; ++i, ++m_pos) {
				if (m_pos < 0) continue;
				sample_t level = size_t(m_pos) < end ? 1.0f - sample_t(m_pos) / end : 0.0f;
				data.rawbuf[i] *= level;
			}
			return ret && m_pos < std::int64_t(end);
		}
	  private:
		callback_t m_stream;
		std::int64_t m_pos;
		double m_time;
	};

	class volume {
	  public:
		volume(sample_t level = 1.0f): m_level(level) {}
		void level(sample_t level) { m_level = level; }
		bool operator()(pcm_data& data) {
			if (m_level == 1.0f) return true;
			for (size_t i = 0, s = data.samples(); i < s; ++i) data.rawbuf[i] *= m_level;
			return true;
		}
	  private:
		sample_t m_level;
	};

	class mutex_stream{
	  public:
	  	mutex_stream(const mutex_stream&) = delete;
  		const mutex_stream& operator=(const mutex_stream&) = delete;
		mutex_stream(callback_t const& stream): m_stream(stream) {}
		bool operator()(pcm_data& data) {
			std::lock_guard<std::recursive_mutex> l(m_mutex);
			if (!m_stream) return false;
			return m_stream(data);
		}
		void clear() {
			std::lock_guard<std::recursive_mutex> l(m_mutex);
			m_stream.clear();
		}
	  private:
		callback_t m_stream;
		mutable std::recursive_mutex m_mutex;
	};

	typedef std::auto_ptr<scoped_lock> lock_holder;

	template <typename Key> class select {
		typedef std::map<Key, callback_t> streams;
	  public:
		void choose(Key const& k) {
			typename streams::iterator it = m_streams.find(k);
			if (it == m_streams.end()) throw std::logic_error("da::select::key: Invalid key");
			m_key = k;
			m_stream = it->second;
		}
		void insert(Key const& k, callback_t const& cb) {
			m_streams[k] = cb;
		}
		bool operator()(pcm_data& data) {
			if (!m_stream) return false;
			return m_stream(data);
		}
	  private:
		streams m_streams;
		Key m_key;
		callback_t m_stream;
	};

	class mixer {
	  public:
		mixer(): m_mutex(std::ref(m_select)) { init(); }
		mixer(settings& s): m_mutex(std::ref(m_select)) { init(); start(s); }
		~mixer() {
			// Make sure that all processing has stopped before exiting
			scoped_lock l(m_mutex);
			m_mutex.clear();
			m_playback.reset();
		}
		void start(settings& s) { m_settings = s; start(); s = m_settings; }
		void start() {
			stop();
			m_settings.set_callback(std::ref(m_mutex));
			m_playback.reset(new playback(m_settings));
		}
		void stop() { m_playback.reset(); }
		bool started() const { return m_playback; }
		void add(callback_t const& cb) {
			scoped_lock l(m_mutex);
			m_user.add(cb);
		}
		void fadein(callback_t const& cb, double time, double startpos = 0.0) {
			scoped_lock l(m_mutex);
			if (m_user.streams.size() <= 1) {
				// The simple case
				m_user.add(cb);
				m_user.add(shared_ref(new fadeinOp(time)));
				return;
			}
			// Make a new chain that produces the same output as the old one
			std::shared_ptr<chain> origch(new chain());
			origch->streams.insert(origch->streams.end(), m_user.streams.begin() + 1, m_user.streams.end());
			// Make a new chain for cb + fade
			std::shared_ptr<chain> fadech(new chain());
			fadech->add(cb);
			fadech->add(shared_ref(new fadeinOp(time, startpos * m_settings.rate() * m_settings.channels())));
			// Replace the output with accumulate of both chains
			clear();
			std::shared_ptr<accumulate> accu(new accumulate());
			accu->add(shared_ref(origch));
			accu->add(shared_ref(fadech));
			add(shared_ref(accu));
		}
		void fadeout(double time, double startpos = 0.0) {
			scoped_lock l(m_mutex);
			if (m_user.streams.size() <= 1) return;
			// Make a new chain that produces the same output as the old one
			std::shared_ptr<chain> ch(new chain());
			ch->streams.insert(ch->streams.end(), m_user.streams.begin() + 1, m_user.streams.end());
			// Replace the output with our new stream (with fade)
			clear();
			add(shared_ref(new fadeoutOp(shared_ref(ch), time, startpos * m_settings.rate() * m_settings.channels())));
		}
		void clear() {
			scoped_lock l(m_mutex);
			m_user.clear();
			m_user.add(voidOp);
		}
		void set_volume(sample_t level) {
			scoped_lock l(m_mutex);
			m_volume.level(level);
		}
		void pause(bool p) {
			scoped_lock l(m_mutex);
			m_select.choose(p ? "paused" : "normal");
		}
		settings get_settings() { return m_settings; }
		lock_holder lock() const { return lock_holder(new scoped_lock(m_mutex)); }
	  private:
		void init() {
			m_master.add(zero);
			m_master.add(std::ref(m_user));
			m_master.add(std::ref(m_volume));
			m_select.insert("paused", zero);
			m_select.insert("normal", std::ref(m_master));
			pause(false);
			clear();
		}
		volume m_volume;
		chain m_user;
		chain m_master;
		select<std::string> m_select;
		mutex_stream m_mutex;
		settings m_settings;
		std::unique_ptr<playback> m_playback;
	};
}
