#pragma once

#include "ui/effect/ieffect.hh"

#include <vector>

class Combination : public IEffect {
public:
	Combination(std::vector<EffectPtr> const&);

	void process(EffectContext&) override;

private:
	std::vector<EffectPtr> m_effects;
};

EffectPtr combine(EffectPtr);
EffectPtr combine(EffectPtr, EffectPtr);
EffectPtr combine(EffectPtr, EffectPtr, EffectPtr);
EffectPtr combine(EffectPtr, EffectPtr, EffectPtr, EffectPtr);
