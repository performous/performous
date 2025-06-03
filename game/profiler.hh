#pragma once

#include "chrono.hh"
#include "log.hh"

#include <fmt/format.h>

#include <iomanip>
#include <iostream>
#include <map>
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

template <>
struct fmt::formatter<ProfCP>: formatter<std::string_view> {
	// Format function
	template <typename FormatContext>
	auto format(const ProfCP& cp, FormatContext& ctx) const{
		std::string ret;
		if (cp.samples == 0) ret = "No data.";
		else if (cp.samples > 1) ret = fmt::format("Samples={}x", cp.samples);
		fmt::format_to(std::back_inserter(ret), "{:.2f} ms average", cp.avg * 1000.0);
		if (cp.peak > 2.0 * cp.avg) fmt::format_to(std::back_inserter(ret), " Peak={:.2f} ms", cp.peak * 1000.0);
		return formatter<std::string_view>::format(ret, ctx);
	}
};

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
	void dump() {
		if (m_checkpoints.empty()) return;
		std::vector<Pair> cps(m_checkpoints.begin(), m_checkpoints.end());
		m_checkpoints.clear();
		std::sort(cps.begin(), cps.end(), cmpFunc);
		std::string prof{fmt::format("PROFILING {}::: ", m_name)};
		for (std::vector<Pair>::const_iterator it = cps.begin(); it != cps.end(); ++it) {
			fmt::format_to(std::back_inserter(prof), "{}: ({}). ", it->first, it->second);
		}
		
		SpdLogger::debug(LogSystem::PROFILER, prof);
	}
};
