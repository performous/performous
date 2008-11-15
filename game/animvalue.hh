#pragma once
#ifndef PERFOURMOUS_ANIMVALUE_HH
#define PERFOURMOUS_ANIMVALUE_HH

#include "xtime.hh"
#include <cmath>
#include <stdexcept>

class AnimValue {
  public:
	AnimValue(double value = 0.0, double rate = 1.0): m_value(value), m_target(value), m_rate(rate), m_time(now()) {}
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


class AnimAcceleration {
  public:
	AnimAcceleration(): m_target(), m_songs(), m_position(), m_velocity(), m_time() {}
	double getValue() {
		const double acceleration = 50.0; // the coefficient of velocity changes (animation speed)
		const double overshoot = 0.95; // Over 1.0 decelerates too late, less than 1.0 decelerates too early
		if (m_songs == 0) return m_target;
		boost::xtime curtime = now();
		double duration = seconds(curtime) - seconds(m_time);
		m_time = curtime;
		if (!(duration > 0.0)) return m_position; // Negative value or NaN, or no songs - skip processing
		if (duration > 1.0) duration = 1.0; // No more than one second per frame
		std::size_t rounds = 1.0 + 1000.0 * duration; // 1 ms or shorter timesteps
		double t = duration / rounds;
		for (std::size_t i = 0; i < rounds; ++i) {
			double d = remainder(m_target - m_position, m_songs); // Distance (via shortest way)
			// Allow it to stop nicely, without jitter
			if (std::abs(m_velocity) < 0.1 && std::abs(d) < 0.001) {
				m_velocity = 0.0;
				m_position = m_target;
				break;
			}
			double a = d > 0.0 ? acceleration : -acceleration; // Acceleration vector
			// Are we going to right direction && can we stop in time if we start decelerating now?
			if (d * m_velocity > 0.0 && std::abs(m_velocity) > 2.0 * overshoot * acceleration * d / m_velocity) a *= -1.0;
			// Apply Newtonian mechanics
			m_velocity += t * a;
			m_position += t * m_velocity;
		}
		return m_position = remainder(m_position, m_songs); // Return & store normalized position
	}
	unsigned int getTarget() const { return m_target; };
	void setTarget(unsigned int target, unsigned int songs) {
		m_time = now();
		m_target = target;
		if (m_songs == songs) return;
		m_songs = songs;
		m_position = target;
	};
  private:
	unsigned int m_target;
	unsigned int m_songs;
	double m_position;
	double m_velocity;
	boost::xtime m_time;
};

#endif

