#include "colorizer.hh"

#include "graphic/color_trans.hh"

namespace {
    struct ImageColorizer : public IImageModification {
        std::shared_ptr<ColorTrans> modification;

        ImageColorizer(std::shared_ptr<ColorTrans> modification)
            : modification(modification) {
        }
    };
}

Colorizer::Colorizer(Color const& color)
: m_color(color) {
}

void Colorizer::setColor(Color const& color) {
    m_color = color;
}

ImageModification Colorizer::modify(Image const& image, Window& window) {
    return std::make_shared<ImageColorizer>(std::make_unique<ColorTrans>(window, m_color));
}


