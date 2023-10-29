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
    ::load(this, filename);
}

Texture::~Texture() {
}

void Texture::load(Bitmap const& bitmap, bool isText) {
    TextureReferenceLoader(*this).load(bitmap, isText);
}

void Texture::update() {
    dimensions = Dimensions(getAspectRatio()).fixedWidth(1.0f);
}

void Texture::draw(Window& window) const {
    if (empty())
        return;

    // FIXME: This gets image alpha handling right but our ColorMatrix system always assumes premultiplied alpha
    // (will produce incorrect results for fade effects)
    glBlendFunc(isPremultiplied() ? GL_ONE : GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw(window, dimensions, TexCoords(tex.x1, tex.y1, tex.x2, tex.y2));
}

void Texture::draw(Window& window, glmath::mat3 const& matrix) const {
    if (empty())
        return;

    // FIXME: This gets image alpha handling right but our ColorMatrix system always assumes premultiplied alpha
    // (will produce incorrect results for fade effects)
    glBlendFunc(isPremultiplied() ? GL_ONE : GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw(window, dimensions, TexCoords(tex.x1, tex.y1, tex.x2, tex.y2), matrix);
}
