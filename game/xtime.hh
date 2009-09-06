#pragma once

#include <boost/thread/xtime.hpp>
#include <cmath>

namespace {
	// Boost WTF time format, directly from C...
	boost::xtime& operator+=(boost::xtime& time, double seconds) {
		double nsec = 1e9 * (time.sec + seconds) + time.nsec;
		time.sec = boost::xtime::xtime_sec_t(nsec / 1e9);
		time.nsec = boost::xtime::xtime_nsec_t(std::fmod(nsec, 1e9));
		return time;
	}
	boost::xtime operator+(boost::xtime const& left, double seconds) {
		boost::xtime time = left;
		return time += seconds;
	}
	double operator-(boost::xtime const& a, boost::xtime const& b) {
		return a.sec - b.sec + 1e-9 * (a.nsec - b.nsec);
	}
	boost::xtime now() {
		boost::xtime time;
		boost::xtime_get(&time, boost::TIME_UTC);
		return time;
	}
	double seconds(boost::xtime const& time) {
		return time.sec + time.nsec * 1e-9;
	}
}
