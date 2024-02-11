#include "texture.hh"

#include "configuration.hh"
#include "graphic/texture_loader.hh"
#include "graphic/video_driver.hh"
#include "screen.hh"
#include "svg.hh"
#include "game.hh"
#include <boost/algorithm/string/case_conv.hpp>

#include <atomic>
#include <cctype>
#include <condition_variable>
#include <stdexcept>
#include <sstream>
#include <thread>
#include <vector>

void updateTextures();

Texture::Texture(fs::path const& filename) { 
    m_loadingId = ::loadTexture(this, filename);
}

Texture::Texture(TextureReferencePtr textureReference)
    : OpenGLTexture(textureReference) {
}

Texture::~Texture() {
    ::abortTextureLoading(m_loadingId);
}

Texture& Texture::clip(unsigned left, unsigned top, unsigned right, unsigned bottom) {
    m_clip.left = left;
    m_clip.top = top;
    m_clip.right = right;
    m_clip.bottom = bottom;

    tex = TexCoords(m_clip, getWidth(), getHeight());

    return *this;
}

void Texture::load(Bitmap const& bitmap, bool isText) {
    TextureReferenceLoader(*this).load(bitmap, isText);

    m_clip = bitmap.clip;

    tex = TexCoords(m_clip, getWidth(), getHeight());
}

void Texture::update() {
    dimensions = Dimensions(getAspectRatio()).fixedWidth(1.0f);
    tex = TexCoords(m_clip, getWidth(), getHeight());
}

void Texture::draw(Window& window) const {
    if (empty())
        return;

    // FIXME: This gets image alpha handling right but our ColorMatrix system always assumes premultiplied alpha
    // (will produce incorrect results for fade effects)
    glBlendFunc(isPremultiplied() ? GL_ONE : GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw(window, dimensions, tex);
}

void Texture::draw(Window& window, glmath::mat3 const& matrix) const {
    if (empty())
        return;

    // FIXME: This gets image alpha handling right but our ColorMatrix system always assumes premultiplied alpha
    // (will produce incorrect results for fade effects)
    glBlendFunc(isPremultiplied() ? GL_ONE : GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw(window, dimensions, tex, matrix);
}
