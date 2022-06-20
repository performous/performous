#pragma once

class EffectManager;

class EffectContext {
public:
	float getSecondsSinceLastFrame() const;
	EffectManager& getManager();

protected:
	EffectContext(float seconds, EffectManager& manager);

private:
	EffectManager& m_manager;
	float m_secondsSinceLastFrame = 0.f;
};
