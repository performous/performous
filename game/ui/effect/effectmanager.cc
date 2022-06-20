#include "ui/effect/effectmanager.hh"

void EffectManager::add(EffectPtr const& effect) {
	m_effects.insert(effect);
}

void EffectManager::remove(EffectPtr const& effect) {
	m_effects.erase(effect);
}

namespace {
	struct PrivateContext : public EffectContext {
		PrivateContext(float secondsSinceLastFrace, EffectManager& manager)
			: EffectContext(secondsSinceLastFrace, manager) {
		}
	};
}

void EffectManager::process(float secondsSinceLastFrame) {
	auto const context = PrivateContext(secondsSinceLastFrame, *this);

	for (auto&& effect : m_effects)
		effect->process(context);
}
