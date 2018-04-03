#pragma once

#include <chrono>

using namespace std::literals::chrono_literals;
using Clock = std::chrono::steady_clock;
using Time = Clock::time_point;
using Seconds = std::chrono::duration<double>;

static inline Clock::duration clockDur(Seconds d) {
	return std::chrono::duration_cast<Clock::duration>(d);
}
