#pragma once

#include "chrono.hh"
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct ProfCP {
	unsigned long samples;
	double total;
	double peak;
	double avg;
	ProfCP(): samples(), total(), peak(), avg() {}
	void add(double t) {
		++samples;
		total += t;
		avg = total / static_cast<double>(samples);
		if (peak < t) peak = t;
	}
};

static inline std::ostream& operator<<(std::ostream& os, ProfCP const& cp) {
	os << std::fixed << std::setprecision(1);
	if (cp.samples == 0) return os << "no data";
	if (cp.samples > 1) os << cp.samples << "x ";
	os << cp.avg * 1000.0 << " ms";
	if (cp.peak > 2.0 * cp.avg) os << " peak " << cp.peak * 1000.0 << " ms";
	return os;
}

/// @short A simple performance profiling tool
class Profiler {
	typedef std::map<std::string, ProfCP> Checkpoints;
	typedef std::pair<std::string, ProfCP> Pair;
	Checkpoints m_checkpoints;
	std::string m_name;
	Time m_time;
	static bool cmpFunc(Pair const& a, Pair const& b) { return a.second.total > b.second.total; }
  public:
	/// Start a profiler with the given name
	Profiler(std::string const& name): m_name(name), m_time(Clock::now()) {}
	~Profiler() { dump(); }
	/// Profiling checkpoint: record the duration since construction or previous checkpoint.
	/// If no tag is specified, no recording is done.
	void operator()(std::string const& tag = std::string()) {
		auto n = Clock::now();
		std::swap(n, m_time);
		double t = Seconds(m_time - n).count();
		m_checkpoints[tag].add(t);
	}
	/// Dump current stats to log and reset
	void dump(std::string const& level = "debug") {
		if (m_checkpoints.empty()) return;
		if (level.empty()) { m_checkpoints.clear(); return; }
		std::vector<Pair> cps(m_checkpoints.begin(), m_checkpoints.end());
		m_checkpoints.clear();
		std::sort(cps.begin(), cps.end(), cmpFunc);
		std::ostringstream oss;
		oss << "profiler-" << m_name << "/" << level << ":";
		for (std::vector<Pair>::const_iterator it = cps.begin(); it != cps.end(); ++it) {
			oss << "  " << it->first << " (" << it->second << ")";
		}
		std::clog << oss.str() << std::endl;
	}
};
