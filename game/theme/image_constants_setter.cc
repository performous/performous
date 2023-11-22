#include "image_constants_setter.hh"

#include "value/constant.hh"

ScopedImageConstantsSetter::ScopedImageConstantsSetter(Theme::Image const& image, Theme&, ConstantValueProviderPtr provider) {
	provider->setValue("image.left", 0.f);
	provider->setValue("image.right", image.texture->dimensions.w());
	provider->setValue("image.center", image.texture->dimensions.w() * 0.5f);
	provider->setValue("image.top", 0.f);
	provider->setValue("image.bottom", image.texture->dimensions.h());
	provider->setValue("image.middle", image.texture->dimensions.h() * 0.5f);
	provider->setValue("image.width", image.texture->dimensions.w());
	provider->setValue("image.height", image.texture->dimensions.h());
}

ScopedImageConstantsSetter::~ScopedImageConstantsSetter() {
	// reset values?
}
