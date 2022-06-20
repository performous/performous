#include "ui/effect/pathchaser.hh"

#include <iostream>

PathChaser::PathChaser(IPathProvider const& pathProvider, float speed)
	: m_pathProvider(pathProvider), m_speed(speed) {
}

void PathChaser::setConsumer(std::function<void(Point const&)> consumer) {
	m_consumer = consumer;
}

void PathChaser::process(EffectContext const& context) {
	if (m_consumer)
		m_consumer(m_pathProvider.getPath()->getPoint(m_position));
//std::cout << "position: " << m_position << std::endl;
	m_position += m_speed * context.getSecondsSinceLastFrame() / m_pathProvider.getPath()->getLength();

	while (m_position < 0.0f)
		m_position += 1.f;
	while (m_position > 1.0f)
		m_position -= 1.f;
}
