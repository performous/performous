#pragma once

#include "chrono.hh"
#include "util.hh"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

/// class for simple/linear animation/transition of values
class AnimValue {
  public:
	/// constructor
	AnimValue(double value = 0.0, double rate = 1.0): m_value(value), m_target(value), m_rate(rate), m_time(Clock::now()) {}
	/// move animation forward by diff
	void move(double diff) { m_value += diff; }
	/// gets animition target
	double getTarget() const { return m_target; }
	/// sets animation target (end point)
	void setTarget(double target, bool step = false) { m_target = target; if (step) m_value = target; }
	/// sets animation range
	void setRange(double tmin, double tmax) {
		if (tmin > tmax) throw std::logic_error("AnimValue range is reversed");
		m_target = clamp(m_target, tmin, tmax);
	}
	/// hard-sets anim value
	void setValue(double value) { m_value = value; }
	/// set the adjustment rate
	void setRate(double value) { m_rate = value; }
	/// get current anim value
	double get() const {
		double maxadj = m_rate * duration();
		double diff = m_value - m_target;
		double adj = std::min(maxadj, std::fabs(diff));
		if (diff > 0.0) m_value -= adj; else m_value += adj;
		return m_value;
	}

  private:
	double duration() const {
		auto newtime = Clock::now();
		Seconds t = newtime - m_time;
		m_time = newtime;
		return clamp(t.count());
	}
	mutable double m_value;
	double m_target;
	double m_rate;
	mutable Time m_time;
};

/// easing animations
class AnimAcceleration {
  public:
	/// constructor
	AnimAcceleration(): m_target(), m_songs(), m_position(), m_velocity(), m_marginLeft(), m_marginRight(), m_time() {}
	/// set margins
	void setMargins(double left, double right) { m_marginLeft = left; m_marginRight = right; }
	/// get current value
	double getValue() {
		const double acceleration = 50.0; // the coefficient of velocity changes (animation speed)
		const double overshoot = 0.95; // Over 1.0 decelerates too late, less than 1.0 decelerates too early
		if (m_songs == 0) return m_target;
		double num = m_marginLeft + m_songs + m_marginRight;
		auto curtime = Clock::now();
		double duration = Seconds(curtime - m_time).count();
		m_time = curtime;
		if (!(duration > 0.0)) return m_position; // Negative value or NaN, or no songs - skip processing
		if (duration > 1.0) {
			// More than one second has elapsed, assume that we have stopped on target already
			m_velocity = 0.0;
			return m_position = m_target;
		}
		std::size_t rounds = static_cast<size_t>(1.0 + 1000.0 * duration); // 1 ms or shorter timesteps
		double t = duration / static_cast<double>(rounds);
		for (std::size_t i = 0; i < rounds; ++i) {
			double d = remainder(m_target - m_position, num); // Distance (via shortest way)
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
		if ((m_position = fmod(m_position, num)) < 0.0) m_position += num; // Normalize to [0, num]
		if (m_position > m_marginLeft + m_songs) m_position -= num; // Normalize to [-m_marginLeft, songs + m_marginRight]
		return m_position;
	}
	/// get target
	std::ptrdiff_t getTarget() const { return static_cast<std::ptrdiff_t>(m_target); };
	/// set target
	void setTarget(std::ptrdiff_t target, std::ptrdiff_t songs) {
		m_target = static_cast<double>(target);
		if (m_songs == songs) return;
		// Number of songs has changed => reset animation
		m_position = static_cast<double>(target);
		m_velocity = 0.0;
	};
	/// resets animation target to 0
	void reset() { setTarget(0, 0); }
	/// get current velocity
	double getVelocity() const { return m_velocity; }

  private:
	double m_target;
	unsigned m_songs;
	double m_position;
	double m_velocity;
	double m_marginLeft;
	double m_marginRight;
	Time m_time;
};
