#ifndef PERFOURMOUS_ANIMVALUE_HH
#define PERFOURMOUS_ANIMVALUE_HH

#include "xtime.hh"

class AnimValue {
  public:
	AnimValue(double value = 0.0, double rate = 1.0): m_value(value), m_target(value), m_rate(rate) {}
	void set(double target) { m_target = target; }
	void setValue(double value) { m_value = value; }
	double get() const {
		for (double t = duration(); t > 0.0; t -= 0.001) {
			double diff = m_value - m_target;
			if (std::abs(diff) < m_rate * 0.001) { m_value = m_target; break; }
			if (diff > 0.0) m_value -= m_rate * 0.001; else m_value += m_rate * 0.001;
		}
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

