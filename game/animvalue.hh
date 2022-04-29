#pragma once

#include "chrono.hh"
#include "util.hh"
#include <cmath>
#include <stdexcept>

/// class for simple/linear animation/transition of values
class AnimValue {
  public:
	/// constructor
	AnimValue(float value = 0.0f, float rate = 1.0f): m_value(value), m_target(value), m_rate(rate), m_time(Clock::now()) {}
	/// move animation forward by diff
	void move(float diff) { m_value += diff; }
	/// gets animition target
	float getTarget() const { return m_target; }
	/// sets animation target (end point)
	void setTarget(float target, bool step = false) { m_target = target; if (step) m_value = target; }
	/// sets animation range
	void setRange(float tmin, float tmax) {
		if (tmin > tmax) throw std::logic_error("AnimValue range is reversed");
		m_target = clamp(m_target, tmin, tmax);
	}
	/// hard-sets anim value
	void setValue(float value) { m_value = value; }
	/// set the adjustment rate
	void setRate(float value) { m_rate = value; }
	/// get current anim value
	float get() const {
		float maxadj = m_rate * duration();
		float diff = m_value - m_target;
		float adj = std::min(maxadj, std::fabs(diff));
		if (diff > 0.0) m_value -= adj; else m_value += adj;
		return m_value;
	}

  private:
	  float duration() const {
		auto newtime = Clock::now();
		Seconds t = newtime - m_time;
		m_time = newtime;
		return clamp(t.count());
	}
	mutable float m_value;
	float m_target;
	float m_rate;
	mutable Time m_time;
};

/// easing animations
class AnimAcceleration {
  public:
	/// constructor
	AnimAcceleration(): m_target(), m_songs(), m_position(), m_velocity(), m_marginLeft(), m_marginRight(), m_time() {}
	/// set margins
	void setMargins(float left, float right) { m_marginLeft = left; m_marginRight = right; }
	/// get current value
	float getValue() {
		const float acceleration = 50.0f; // the coefficient of velocity changes (animation speed)
		const float overshoot = 0.95f; // Over 1.0 decelerates too late, less than 1.0 decelerates too early
		if (m_songs == 0) return m_target;
		float num = m_marginLeft + m_songs + m_marginRight;
		auto curtime = Clock::now();
		float duration = Seconds(curtime - m_time).count();
		m_time = curtime;
		if (!(duration > 0.0f)) return m_position; // Negative value or NaN, or no songs - skip processing
		if (duration > 1.0f) {
			// More than one second has elapsed, assume that we have stopped on target already
			m_velocity = 0.0f;
			return m_position = m_target;
		}
		std::size_t rounds = 1.0f + 1000.0f * duration; // 1 ms or shorter timesteps
		float t = duration / rounds;
		for (std::size_t i = 0; i < rounds; ++i) {
			float d = remainder(m_target - m_position, num); // Distance (via shortest way)
			// Allow it to stop nicely, without jitter
			if (std::abs(m_velocity) < 0.1 && std::abs(d) < 0.001f) {
				m_velocity = 0.0;
				m_position = m_target;
				break;
			}
			float a = d > 0.0f ? acceleration : -acceleration; // Acceleration vector
			// Are we going to right direction && can we stop in time if we start decelerating now?
			if (d * m_velocity > 0.0f && std::abs(m_velocity) > 2.0f * overshoot * acceleration * d / m_velocity) a *= -1.0f;
			// Apply Newtonian mechanics
			m_velocity += t * a;
			m_position += t * m_velocity;
		}
		if ((m_position = fmod(m_position, num)) < 0.0f) m_position += num; // Normalize to [0, num]
		if (m_position > m_marginLeft + m_songs) m_position -= num; // Normalize to [-m_marginLeft, songs + m_marginRight]
		return m_position;
	}
	/// get target
	unsigned int getTarget() const { return m_target; };
	/// set target
	void setTarget(unsigned int target, unsigned int songs) {
		m_target = target;
		if (m_songs == songs) return;
		// Number of songs has changed => reset animation
		m_songs = songs;
		m_position = target;
		m_velocity = 0.0f;
	};
	/// resets animation target to 0
	void reset() { setTarget(0, 0); }
	/// get current velocity
	float getVelocity() const { return m_velocity; }

  private:
	unsigned int m_target;
	unsigned int m_songs;
	float m_position;
	float m_velocity;
	float m_marginLeft;
	float m_marginRight;
	Time m_time;
};
