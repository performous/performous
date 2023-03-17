#pragma once

#include "ui/effect/ieffect.hh"

#include <set>

class GraphicContext;

class EffectManager {
public:
	void add(EffectPtr const&);
	void remove(EffectPtr const&);

	void process(GraphicContext&, float secondsSinceLastFrame, float secondsSinceStart);

private:
	std::set<EffectPtr> m_effects;
};
