#include "texture_manager.hh"

#include "texture_loader.hh"
#include "json.hh"

namespace {
    struct ImageDescription {
        fs::path path;
        unsigned left;
        unsigned top;
        unsigned right;
        unsigned bottom;
    };

    ImageDescription getImageDescription(fs::path const& path) {
        if (path.extension() != ".json") {
            return { path, 0, 0, 0, 0 };
        }

        auto const json = readJSON(path);

        if (!json.is_object())
            return { path, 0, 0, 0, 0 };

        return {
            findFile(json.at("filename").get<std::string>()),
            json.at("left").get<unsigned>(),
            json.at("top").get<unsigned>(),
            json.at("right").get<unsigned>(),
            json.at("bottom").get<unsigned>()
        };
    }
}

TextureManager::TextureManager() {
//    get(findFile("patterns-warning-black-yellow.png"));
}

Texture TextureManager::get(fs::path const& filepath) {
    auto const desciption = getImageDescription(filepath);
    auto const filename = desciption.path.string();

    {
        auto guard = std::lock_guard<std::mutex>(m_mutex);
        auto const it = m_fileTextureMap.find(filename);

        if (it != m_fileTextureMap.end()) {
            return Texture(it->second).clip(desciption.left, desciption.top, desciption.right, desciption.bottom);
        }
    }

    auto reference = std::make_shared<TextureReference>();

    {
        auto guard = std::lock_guard<std::mutex>(m_mutex);
        m_fileTextureMap[filename] = reference;
    }

    ::loadTexture(this, desciption.path);

    return Texture(reference).clip(desciption.left, desciption.top, desciption.right, desciption.bottom);
}

void TextureManager::load(Bitmap const& bitmap, bool isText) {
    auto const filename = bitmap.filepath.string();

    auto guard = std::lock_guard<std::mutex>(m_mutex);
    auto it = m_fileTextureMap.find(filename);

    if (it == m_fileTextureMap.end())
        return;

    auto referencePtr = it->second;

    TextureReferenceLoader(referencePtr).load(bitmap, isText);
}

