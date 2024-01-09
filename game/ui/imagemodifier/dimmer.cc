#include "dimmer.hh"

#include "color.hh"
#include "graphic/color_trans.hh"

namespace {
    struct ImageDimming : public IImageModification  {
        std::shared_ptr<ColorTrans> modification;

        ImageDimming(std::shared_ptr<ColorTrans> modification)
            : modification(modification) {
        }
    };
}

Dimmer::Dimmer(float value)
    : m_value(value) {
}

void Dimmer::dimm(float value) {
    m_value = value;
}

ImageModification Dimmer::modify(Image const& image, Window& window) {
    return std::make_shared<ImageDimming>(std::make_unique<ColorTrans>(window, Color::alpha(m_value)));
}
