#pragma once

#include "ui/effect/ieffect.hh"

#include <set>

class GraphicContext;

class EffectManager {
public:
	EffectManager(GraphicContext&);

	void add(EffectPtr const&);
	void remove(EffectPtr const&);

	void process(float secondsSinceLastFrame, float secondsSinceStart);

private:
	GraphicContext& m_graphiccontext;
	std::set<EffectPtr> m_effects;
};
