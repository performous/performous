#include "ui/effect/sinus.hh"

#include <cmath>

Sinus::Sinus(float durationInSeconds, float min, float max)
: m_durationInSeconds(durationInSeconds), m_min(min), m_max(max) {
}

void Sinus::setConsumer(std::function<void(float)> consumer) {
	m_consumer = consumer;
}

namespace {
	float const pi(atanf(1.f) * 4.f);
}

void Sinus::process(EffectContext const& context) {
	if(m_consumer) {
		auto const difference = m_max - m_min;
		auto const v = context.getSecondsSinceStart() * 2.f * pi / m_durationInSeconds;
		m_consumer((0.5f + 0.5f * std::sin(v)) * difference + m_min);
	}
}
