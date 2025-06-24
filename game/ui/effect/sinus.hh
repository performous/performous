#pragma once

#include "ui/effect/ieffect.hh"

#include <functional>

class Sinus : public IEffect {
public:
	Sinus(float durationInSeconds = 1.f, float min = -1.f, float max = 1.f);

	void setConsumer(std::function<void(float, EffectContext& context)>);

	void process(EffectContext&) override;

private:
	float m_durationInSeconds = 1.f;
	float m_min = 0.f;
	float m_max = 1.f;
	std::function<void(float, EffectContext& context)> m_consumer;
};


