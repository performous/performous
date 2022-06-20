#pragma once

#include "ui/effect/ieffect.hh"

#include <functional>

class Rotation : public IEffect {
public:
	Rotation(float speed = 0.25f);

	void setConsumer(std::function<void(float)>);

	void process(EffectContext const&) override;

private:
	float m_speed = 0.25f;
	std::function<void(float)> m_consumer;
	float m_angle = 0.f;
};

