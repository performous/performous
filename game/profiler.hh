#pragma once

#include "xtime.hh"
#include <iostream>
#include <sstream>
#include <string>

/// easy access for profiling code
class Profiler {
	std::ostringstream m_oss;
	boost::xtime m_time;
  public:
  /// create a new profiler with a given name
	Profiler(std::string const& name): m_time(now()) { m_oss << "profiler-" << name << "/info: "; }
	~Profiler() { std::clog << m_oss.str() << std::endl; }
	/// calling the object as a function will return the time since the start
	void operator()(std::string const& tag) {
		boost::xtime n = now();
		std::swap(n, m_time);
		m_oss << unsigned((m_time - n) * 1000.0 + 0.5) << " ms (" << tag << ")  ";
	}
};


