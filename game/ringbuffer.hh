#pragma once

#include <atomic>

/// Lock-free ring buffer. Discards oldest data on overflow (not strictly thread-safe).
template <std::size_t SIZE> class RingBuffer {
public:
	constexpr static std::size_t capacity = SIZE;
	RingBuffer(): m_read(), m_write() {}  ///< Initialize empty buffer
	template <typename InIt> void insert(InIt begin, InIt end) {
		unsigned r = m_read;  // The read position
		unsigned w = m_write;  // The write position
		bool overflow = false;
		while (begin != end) {
			m_buf[w] = *begin++;  // Copy sample
			w = modulo(w + 1);  // Update cursor
			if (w == r) overflow = true;
		}
		m_write = w;
		if (overflow) m_read = modulo(w + 1);  // Reset read pointer on overflow
	}
	/// Read data from current position if there is enough data to fill the range (otherwise return false). Does not move read pointer.
	template <typename OutIt> bool read(OutIt begin, OutIt end) {
		unsigned r = m_read;
		if (modulo(m_write - r) <= end - begin) return false;  // Not enough audio available
		while (begin != end) *begin++ = m_buf[r++ % SIZE];  // Copy audio to output iterator
		return true;
	}
	void pop(unsigned n) { m_read = modulo(m_read + n); } ///< Move reading pointer forward.
	unsigned size() const { return modulo(m_write - m_read); }

private:
	static unsigned modulo(unsigned idx) { return (SIZE + idx) % SIZE; }  ///< Modulo operation with proper rounding (handles slightly "negative" idx as well)
	float m_buf[SIZE];
	// The indices of the next read/write operations. read == write implies that buffer is empty.
	std::atomic<unsigned> m_read{ 0 };
	std::atomic<unsigned> m_write{ 0 };
};

