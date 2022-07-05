#pragma once

class EffectManager;
class GraphicContext;

class EffectContext {
public:
	float getSecondsSinceLastFrame() const;
	float getSecondsSinceStart() const;
	EffectManager& getManager();

	GraphicContext& getGraphicContext();

protected:
	EffectContext(float secondsSinceLastFrame, float secondsSinceStart, EffectManager& manager, GraphicContext&);

private:
	GraphicContext& m_graphiccontext;
	EffectManager& m_manager;
	float m_secondsSinceLastFrame = 0.f;
	float m_secondsSinceStart = 0.f;
};
