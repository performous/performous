#include "ui/effect/effectmanager.hh"

EffectManager::EffectManager(GraphicContext& gc)
: m_graphiccontext(gc) {
}

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

void EffectManager::process(float secondsSinceLastFrame, float secondsSinceStart) {
	auto context = PrivateContext(secondsSinceLastFrame, secondsSinceStart, *this, m_graphiccontext);

	for (auto&& effect : m_effects)
		effect->process(context);
}
