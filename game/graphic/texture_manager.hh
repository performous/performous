#pragma once

#include "texture.hh"
#include "texture_reference.hh"
#include "utils/non_copyable.hh"

#include <map>
#include <mutex>
#include <string>

class TextureManager : public NonCopyable {
public:
    TextureManager();

    Texture get(fs::path const&);

    void load(Bitmap const& bitmap, bool isText);

private:
    std::map<std::string, TextureReferencePtr> m_fileTextureMap;
    std::mutex m_mutex;
};
