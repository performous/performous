#pragma once

#include <atomic>
#include <cstddef>

/// Lock-free ring buffer. Discards oldest data on overflow (not strictly thread-safe).
template <std::ptrdiff_t SIZE> class RingBuffer {
  public:
	constexpr static std::ptrdiff_t capacity = SIZE;

	template <typename InIt> void insert(InIt begin, InIt end);
	/// Read data from current position if there is enough data to fill the range (otherwise return false). Does not move read pointer.
	template <typename OutIt> bool read(OutIt begin, OutIt end);
	void pop(std::ptrdiff_t n); ///< Move reading pointer forward.
	std::ptrdiff_t size() const;

  private:
	static std::ptrdiff_t modulo(std::ptrdiff_t idx);  ///< Modulo operation with proper rounding (handles slightly "negative" idx as well)

	constexpr static std::ptrdiff_t buffersize = SIZE + 1;
	float m_buf[buffersize];
	// The indices of the next read/write operations. read == write implies that buffer is empty.
	std::atomic<std::ptrdiff_t> m_read{ 0 };
	std::atomic<std::ptrdiff_t> m_write{ 0 };
};

template <std::ptrdiff_t SIZE>
template <typename InIt>
void RingBuffer<SIZE>::insert(InIt begin, InIt end) {
	std::ptrdiff_t r = m_read;  // The read position
	std::ptrdiff_t w = m_write;  // The write position
	bool overflow = false;
	while (begin != end) {
		m_buf[w] = *begin++;  // Copy sample
		w = modulo(w + 1);  // Update cursor
		if (w == r)
			overflow = true;
	}
	m_write = w;
	if (overflow)
		m_read = modulo(w + 1);  // Reset read pointer on overflow
}

template <std::ptrdiff_t SIZE>
template <typename OutIt>
bool RingBuffer<SIZE>::read(OutIt begin, OutIt end) {
	std::ptrdiff_t requestedELements = end - begin;
	if (size() < requestedELements) // Not enough audio available
		return false;
	std::ptrdiff_t r = m_read;
	while (begin != end)  // Copy audio to output iterator
		*begin++ = m_buf[r++ % buffersize];
	return true;
}

template <std::ptrdiff_t SIZE>
void RingBuffer<SIZE>::pop(std::ptrdiff_t n) {
	m_read = modulo(m_read + n);
}

template <std::ptrdiff_t SIZE>
std::ptrdiff_t RingBuffer<SIZE>::size() const {
	return modulo(m_write - m_read);
}

template <std::ptrdiff_t SIZE>
std::ptrdiff_t RingBuffer<SIZE>::modulo(std::ptrdiff_t idx) {
	return (buffersize + idx) % buffersize;
}
