#include "ui/effect/combination.hh"

#include <memory>

Combination::Combination(const std::vector<EffectPtr>& effects)
: m_effects(effects) {
}

void Combination::process(EffectContext const& context) {
	for(auto const& effect : m_effects)
		effect->process(context);
}

EffectPtr combine(EffectPtr e0) {
	return std::make_shared<Combination>(std::vector<EffectPtr>{e0});
}

EffectPtr combine(EffectPtr e0, EffectPtr e1) {
	return std::make_shared<Combination>(std::vector<EffectPtr>{e0, e1});
}

EffectPtr combine(EffectPtr e0, EffectPtr e1, EffectPtr e2) {
	return std::make_shared<Combination>(std::vector<EffectPtr>{e0, e1, e2});
}

EffectPtr combine(EffectPtr e0, EffectPtr e1, EffectPtr e2, EffectPtr e3) {
	return std::make_shared<Combination>(std::vector<EffectPtr>{e0, e1, e2, e3});
}
