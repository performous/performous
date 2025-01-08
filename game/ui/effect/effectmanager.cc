#include "ui/effect/effectmanager.hh"

void EffectManager::add(EffectPtr const& effect) {
	m_effects.insert(effect);
}

void EffectManager::remove(EffectPtr const& effect) {
	m_effects.erase(effect);
}

namespace {
	struct PrivateContext : public EffectContext {
		PrivateContext(float secondsSinceLastFrace, float secondsSinceStart, EffectManager& manager, GraphicContext& gc)
			: EffectContext(secondsSinceLastFrace, secondsSinceStart, manager, gc) {
		}
	};
}

void EffectManager::process(GraphicContext& gc, float secondsSinceLastFrame, float secondsSinceStart) {
	auto context = PrivateContext(secondsSinceLastFrame, secondsSinceStart, *this, gc);

	for (auto&& effect : m_effects)
		effect->process(context);
}
