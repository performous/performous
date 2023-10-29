#pragma once

#include "dimensions.hh"
#include "texture_reference.hh"
#include "texture_coordinates.hh"
#include "shader_manager.hh"

Shader& getShader(Window&, std::string const& name);

template <GLenum Type> class OpenGLTexture {
public:
	OpenGLTexture();
	OpenGLTexture(TextureReferencePtr texture);
	OpenGLTexture(OpenGLTexture const&) = default;
	~OpenGLTexture() = default;

	OpenGLTexture& operator=(const OpenGLTexture&) = default;

	static GLenum type() { return Type; };
	static Shader& shader(Window& window) { return getShader(window, "texture"); }


	/// returns id
	GLuint id() const { return getId(); };
	GLuint getId() const { return m_textureReference->getId(); };

	/// draw in given dimensions, with given texture coordinates
	void draw(Window&, Dimensions const& dim, TexCoords const& tex = TexCoords()) const;
	void draw(Window&, Dimensions const& dim, TexCoords const& tex, glmath::mat3 const& matrix) const;
	/// draw a subsection of the orig dimensions, cropping by tex
	void drawCropped(Window&, Dimensions const& orig, TexCoords const& tex) const;

	float width() const { return m_textureReference->getWidth(); }
	float height() const { return m_textureReference->getHeight(); }
	float getWidth() const { return m_textureReference->getWidth(); }
	float getHeight() const { return m_textureReference->getHeight(); }
	float getAspectRatio() const { return m_textureReference->getAspectRatio(); }
	bool isPremultiplied() const { return m_textureReference->isPremultiplied(); }

	operator TextureReferencePtr() const { return m_textureReference; }

private:
	TextureReferencePtr m_textureReference;
};


/** @short A RAII wrapper for binding to a texture (using it, modifying it) **/
class UseTexture {
public:
	UseTexture(const UseTexture&) = delete;
	const UseTexture& operator=(const UseTexture&) = delete;
	/// constructor
	template <GLenum Type> UseTexture(Window& window, OpenGLTexture<Type> const& tex) :
		m_shader(
			/* hack of the year */
			(glutil::GLErrorChecker("UseTexture"), glActiveTexture(GL_TEXTURE0),
				glBindTexture(Type, tex.id()), tex.shader(window))) {
	}

private:
	UseShader m_shader;
};

template <GLenum Type> OpenGLTexture<Type>::OpenGLTexture() {
	m_textureReference = std::make_shared<TextureReference>(); 
}

template <GLenum Type> OpenGLTexture<Type>::OpenGLTexture(TextureReferencePtr texture)
	: m_textureReference(texture) {
}

template <GLenum Type> void OpenGLTexture<Type>::draw(Window& window, Dimensions const& dim, TexCoords const& tex) const {
	glutil::GLErrorChecker glerror("OpenGLTexture::draw()");
	glutil::VertexArray va;

	auto const&& binder = UseTexture(window, *this);
	glerror.check("texture");

	// The texture wraps over at the edges (repeat)
	const bool repeating = tex.outOfBounds();
	glTexParameterf(type(), GL_TEXTURE_WRAP_S, repeating ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameterf(type(), GL_TEXTURE_WRAP_T, repeating ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glerror.check("repeat mode");

	va.texCoord(tex.x1, tex.y1).vertex(dim.x1(), dim.y1());
	va.texCoord(tex.x2, tex.y1).vertex(dim.x2(), dim.y1());
	va.texCoord(tex.x1, tex.y2).vertex(dim.x1(), dim.y2());
	va.texCoord(tex.x2, tex.y2).vertex(dim.x2(), dim.y2());

	va.draw();
}

template <GLenum Type> void OpenGLTexture<Type>::draw(Window& window, Dimensions const& dim, TexCoords const& tex, glmath::mat3 const& matrix) const {
	glutil::GLErrorChecker glerror("OpenGLTexture::draw()");
	glutil::VertexArray va;

	auto&& binder = UseTexture(window, *this);
	glerror.check("texture");

	// The texture wraps over at the edges (repeat)
	const bool repeating = tex.outOfBounds();
	glTexParameterf(type(), GL_TEXTURE_WRAP_S, repeating ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameterf(type(), GL_TEXTURE_WRAP_T, repeating ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glerror.check("repeat mode");

	auto const v0 = matrix * glmath::vec3(dim.x1(), dim.y1(), 1);
	auto const v1 = matrix * glmath::vec3(dim.x2(), dim.y1(), 1);
	auto const v2 = matrix * glmath::vec3(dim.x1(), dim.y2(), 1);
	auto const v3 = matrix * glmath::vec3(dim.x2(), dim.y2(), 1);

	va.texCoord(tex.x1, tex.y1).vertex(v0);
	va.texCoord(tex.x2, tex.y1).vertex(v1);
	va.texCoord(tex.x1, tex.y2).vertex(v2);
	va.texCoord(tex.x2, tex.y2).vertex(v3);

	va.draw();
}

template <GLenum Type> void OpenGLTexture<Type>::drawCropped(Window& window, Dimensions const& orig, TexCoords const& tex) const {
	Dimensions dim(
		orig.x1() + tex.x1 * orig.w(),
		orig.y1() + tex.y1 * orig.h(),
		orig.w() * (tex.x2 - tex.x1),
		orig.h() * (tex.y2 - tex.y1)
	);
	draw(window, dim, tex);
}
