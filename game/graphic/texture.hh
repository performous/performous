#pragma once

#include "graphic/glutil.hh"
#include "graphic/bitmap.hh"
#include "graphic/window.hh"
#include "graphic/dimensions.hh"
#include "graphic/texture_coordinates.hh"
#include "graphic/opengl_texture.hh"

#include <cairo.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

Shader& getShader(Window&, std::string const& name);
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

	bool empty() const { return m_width * m_height == 0.f; } ///< Test if the loading has failed
	/// draws texture
	void draw(Window&) const;
	void draw(Window&, glmath::mat3 const&) const;
	using OpenGLTexture<GL_TEXTURE_2D>::draw;
	/// loads texture into buffer
	void load(Bitmap const& bitmap, bool isText = false);
	Shader& shader(Window& window) { return OpenGLTexture::shader(window); }
	float width() const { return m_width; }
	float height() const { return m_height; }

private:
	float m_width = 0.f;
	float m_height = 0.f;
	bool m_premultiplied = true;
};

/// A RAII wrapper for texture loading worker thread. There must be exactly one (global) instance whenever any Textures exist.
class TextureLoader {
public:
	TextureLoader();
	~TextureLoader();
	class Impl;
};

