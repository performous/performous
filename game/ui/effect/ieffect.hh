#pragma once

#include "ui/effect/effectcontext.hh"

#include <memory>

struct IEffect {
	virtual ~IEffect() = default;

	virtual void process(EffectContext&) = 0;
};

using EffectPtr = std::shared_ptr<IEffect>;
