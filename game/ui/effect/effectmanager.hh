#pragma once

#include "ui/effect/ieffect.hh"

#include <set>

class EffectManager {
public:
	void add(EffectPtr const&);
	void remove(EffectPtr const&);

	void process(float secondsSinceLastFrame);

private:
	std::set<EffectPtr> m_effects;
};
