#pragma once

#include "xtime.hh"
#include <iostream>
#include <sstream>
#include <string>

class Profiler {
	std::ostringstream m_oss;
	boost::xtime m_time;
  public:
	Profiler(std::string const& name): m_time(now()) { m_oss << name << ": "; }
	~Profiler() { std::clog << m_oss.str() << std::endl; }
	void operator()(std::string const& tag) {
		boost::xtime n = now();
		std::swap(n, m_time);
		m_oss << unsigned((m_time - n) * 1000.0 + 0.5) << " ms (" << tag << ")  ";
	}
};


