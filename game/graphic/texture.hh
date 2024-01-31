#pragma once

#include "graphic/glutil.hh"
#include "graphic/bitmap.hh"
#include "graphic/window.hh"
#include "graphic/dimensions.hh"
#include "graphic/texture_coordinates.hh"
#include "graphic/texture_loader.hh"
#include "graphic/opengl_texture.hh"

#include <cairo.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

void updateTextures();

/**
* @short High level texture/image wrapper on top of OpenGLTexture
**/
class Texture: public OpenGLTexture<GL_TEXTURE_2D> {
public:
	struct Impl;
	/// dimensions
	Dimensions dimensions;
	/// texture coordinates
	TexCoords tex;
	Texture() = default;
	/// creates texture from file
	Texture(fs::path const& filename);
	Texture(TextureReferencePtr textureReference) : OpenGLTexture(textureReference) {}
	~Texture();

	bool empty() const { return width() * height() == 0.f; } ///< Test if the loading has failed
	/// draws texture
	void draw(Window&) const;
	void draw(Window&, glmath::mat3 const&) const;
	using OpenGLTexture<GL_TEXTURE_2D>::draw;
	/// loads texture into buffer
	void load(Bitmap const& bitmap, bool isText = false);
	Shader& shader(Window& window) { return OpenGLTexture::shader(window); }
	float width() const { return getWidth(); }
	float height() const { return getHeight(); }

	void update() override;

private:
	TextureLoadingId m_loadingId{ TextureLoadingId(-1)};
};

