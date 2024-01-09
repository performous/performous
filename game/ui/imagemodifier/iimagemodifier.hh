#pragma once

#include <memory>

class Image;
class Window;

struct IImageModification {
    virtual ~IImageModification() = default;
};

using ImageModification = std::shared_ptr<IImageModification>;

struct IImageModifier {
    virtual ~IImageModifier() = default;

    virtual ImageModification modify(Image const&, Window&) = 0;
};

using ImageModifier = std::shared_ptr<IImageModifier>;
