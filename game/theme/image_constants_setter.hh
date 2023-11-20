#pragma once

#include "theme.hh"

class ScopedImageConstantsSetter {
public:
	ScopedImageConstantsSetter(Theme::Image const& image, Theme& theme, ConstantValueProviderPtr);
	~ScopedImageConstantsSetter();

private:
	Theme::Image const& m_image;
	Theme& m_theme;
	ConstantValueProviderPtr m_provider;
};
