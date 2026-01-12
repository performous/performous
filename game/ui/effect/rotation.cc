#include "ui/effect/rotation.hh"

#include <cmath>

Rotation::Rotation(float speed)
:  m_speed(speed) {
}

void Rotation::setConsumer(std::function<void(float)> consumer) {
	m_consumer = consumer;
}

namespace {
#ifdef GLM_FORCE_RADIANS
	float const pi(atanf(1.f) * 4.f);
#endif
}

void Rotation::process(EffectContext& context) {
	if (m_consumer)
#ifdef GLM_FORCE_RADIANS
		m_consumer(m_angle * pi * 2.f);
#else
		m_consumer(m_angle * 360.f);
#endif

	m_angle += m_speed * context.getSecondsSinceLastFrame();

	while (m_angle < 0.0f)
		m_angle += 1.f;
	while (m_angle > 1.0f)
		m_angle -= 1.f;
}
