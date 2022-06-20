#pragma once

#include "ui/effect/ieffect.hh"
#include "ui/path/ipathprovider.hh"

#include <functional>

class PathChaser : public IEffect {
public:
	PathChaser(IPathProvider const&, float speed = 0.25f);

	void setConsumer(std::function<void(Point const&)>);

	void process(EffectContext const&) override;

private:
	IPathProvider const& m_pathProvider;
	float m_speed = 0.25f;
	std::function<void(Point const&)> m_consumer;
	float m_position = 0.f;
};
