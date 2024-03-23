#pragma once

#include "iimagemodifier.hh"

class Dimmer : public IImageModifier {
public:
	Dimmer(float value = 1.f);

	void dimm(float value);

	ImageModification modify(Image const&, Window&) override;

private:
	float m_value = 1.f; // no dimming
};
