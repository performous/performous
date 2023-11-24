#include "image_constants_setter.hh"

#include "value/constant.hh"

ScopedImageConstantsSetter::ScopedImageConstantsSetter(Theme::Image const& image, Theme& theme, ConstantValueProviderPtr provider) {
	provider->setValue("image.left", image.texture->dimensions.x1());
	provider->setValue("image.center", image.texture->dimensions.xc());
	provider->setValue("image.right", image.texture->dimensions.x2());
	provider->setValue("image.top", image.texture->dimensions.y1());
	provider->setValue("image.middle", image.texture->dimensions.yc());
	provider->setValue("image.bottom", image.texture->dimensions.y2());
	provider->setValue("image.width", image.texture->dimensions.w());
	provider->setValue("image.height", image.texture->dimensions.h());
	provider->setValue("image.width_half", image.texture->dimensions.w() * 0.5f);
	provider->setValue("image.height_half", image.texture->dimensions.h() * 0.5f);
	provider->setValue("image.width_origin", image.texture->dimensions.getWidth(false));
	provider->setValue("image.height_origin", image.texture->dimensions.getHeight(false));

	for (auto const& item : theme.values) {
		std::cout << "value " << item.first << ": " << item.second.get() << std::endl;

		if (item.first == "logo_height")
			item.second.get();
	}
}

ScopedImageConstantsSetter::~ScopedImageConstantsSetter() {
	// reset values?
}
