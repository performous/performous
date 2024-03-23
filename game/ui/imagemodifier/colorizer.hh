#pragma once

#include "iimagemodifier.hh"

#include "color.hh"

class Colorizer : public IImageModifier {
public:
	Colorizer(Color const& = Color::alpha(1.f));

	void setColor(Color const&);

	ImageModification modify(Image const&, Window&) override;

private:
	Color m_color;
};
