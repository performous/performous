#pragma once

struct IImageLoader {
    virtual ~IImageLoader() = default;

    virtual bool canLoad(fs::path const&) const = 0;
    virtual Bitmap load(fs::path const&) const = 0;
};

using ImageLoaderPtr = std::shared_ptr<IImageLoader>;

struct ImageReceiver {
    void receiveImage()
};

class ImageLoader {
public:
    ImageLoader& add(ImageLoaderPtr);

    void load(fs::path const& path, ImageReceiver&);

private:
    std::set<ImageLoaderPtr> m_loaders;
};
