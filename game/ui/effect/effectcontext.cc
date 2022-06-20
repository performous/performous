#include "ui/effect/effectcontext.hh"

EffectContext::EffectContext(float seconds, EffectManager& manager)
	: m_manager(manager), m_secondsSinceLastFrame(seconds) {
}

float EffectContext::getSecondsSinceLastFrame() const {
	return m_secondsSinceLastFrame;
}

EffectManager& EffectContext::getManager() {
	return m_manager;
}
