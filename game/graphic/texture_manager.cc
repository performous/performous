#include "texture_manager.hh"

#include "texture_loader.hh"

TextureManager::TextureManager() {
//    get(findFile("patterns-warning-black-yellow.png"));
}

Texture TextureManager::get(fs::path const& filepath) {
    auto const filename = filepath.string();

    {
        auto guard = std::lock_guard<std::mutex>(m_mutex);
        auto const it = m_fileTextureMap.find(filename);

        if (it != m_fileTextureMap.end()) {
            return Texture(it->second);
        }
    }

    auto reference = std::make_shared<TextureReference>();

    {
        auto guard = std::lock_guard<std::mutex>(m_mutex);
        m_fileTextureMap[filename] = reference;
    }

    ::loadTexture(this, filepath);

    return Texture(reference);
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

