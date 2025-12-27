#include "ui/effect/generic.hh"

#include <iostream>

Generic::Generic(std::function<void(EffectContext&)> callback)
: m_callback(callback) {
}

void Generic::process(EffectContext& context) {
	if(m_callback)
		m_callback(context);
}
