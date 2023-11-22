#pragma once

#include "theme.hh"

class ScopedImageConstantsSetter {
public:
	ScopedImageConstantsSetter(Theme::Image const& image, Theme& theme, ConstantValueProviderPtr);
	~ScopedImageConstantsSetter();

private:
};
