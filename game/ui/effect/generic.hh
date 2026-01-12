#pragma once

#include "ui/effect/ieffect.hh"

#include <functional>

class Generic : public IEffect {
public:
	Generic(std::function<void(EffectContext&)> callback);

	void process(EffectContext& context) override;

private:
	std::function<void(EffectContext&)> m_callback;
};


