#ifndef PERFOURMOUS_ANIMVALUE_HH
#define PERFOURMOUS_ANIMVALUE_HH

#include "xtime.hh"
#include <cmath>
#include <stdexcept>

class AnimValue {
  public:
	AnimValue(double value = 0.0, double rate = 1.0): m_value(value), m_target(value), m_rate(rate) {}
	void move(double diff) { m_value += diff; }
	double getTarget() const { return m_target; }
	void setTarget(double target, bool step = false) { m_target = target; if (step) m_value = target; }
	void setRange(double tmin, double tmax) {
		if (tmin > tmax) throw std::logic_error("AnimValue range is reversed");
		m_target = std::min(std::max(tmin, m_target), tmax);
	}
	void setValue(double value) { m_value = value; }
	double get() const {
		double maxadj = m_rate * duration();
		double diff = m_value - m_target;
		double adj = std::min(maxadj, std::fabs(diff));
		if (diff > 0.0) m_value -= adj; else m_value += adj;
		return m_value;
	}
  private:
	double duration() const {
		boost::xtime newtime = now();
		double t = newtime - m_time;
		m_time = newtime;
		return std::min(std::max(0.0, t), 1.0);
	}
	mutable double m_value;
	double m_target;
	double m_rate;
	mutable boost::xtime m_time;
};

#endif

