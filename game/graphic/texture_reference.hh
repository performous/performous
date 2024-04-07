#pragma once

#include "utils/change_notifier.hh"

#include <memory>

#include <epoxy/gl.h>

struct TextureReference : public ChangeNotifier {
    TextureReference();
    TextureReference(const TextureReference&) = delete;
    ~TextureReference();

    TextureReference& operator=(const TextureReference&) = delete;

    GLuint getId() const;

    float getWidth() const;
    float getHeight() const;
    TextureReference& setGeometry(float width, float height);
    TextureReference& setGeometry(float width, float height, float aspectRatio);
    float getAspectRatio() const;
    bool isPremultiplied() const;
    TextureReference& setPremultiplied(bool = true);

    void changed();

private:
    GLuint m_id;
    float m_width = 0.f;
    float m_height = 0.f;
    float m_aspectRatio = 1.f;
    bool m_premultiplied = true;
};

using TextureReferencePtr = std::shared_ptr<TextureReference>;
