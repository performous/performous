#include "ui/effect/effectcontext.hh"

EffectContext::EffectContext(float secondsSinceLastFrame, float secondsSinceStart, EffectManager& manager, GraphicContext& gc)
	: m_graphiccontext(gc), m_manager(manager), m_secondsSinceLastFrame(secondsSinceLastFrame), m_secondsSinceStart(secondsSinceStart) {
}

float EffectContext::getSecondsSinceLastFrame() const {
	return m_secondsSinceLastFrame;
}

float EffectContext::getSecondsSinceStart() const {
	return m_secondsSinceStart;
}

EffectManager& EffectContext::getManager() {
	return m_manager;
}

GraphicContext& EffectContext::getGraphicContext() {
	return m_graphiccontext;
}

