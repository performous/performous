#pragma once

#include "dimensions.hh"
#include "texture_reference.hh"
#include "texture_coordinates.hh"
#include "shader_manager.hh"
#include "window.hh"

#include <glm/gtx/matrix_transform_2d.hpp>

template <GLenum Type> class OpenGLTexture : public ChangeReceiver {
public:
	OpenGLTexture();
	OpenGLTexture(TextureReferencePtr texture);
	OpenGLTexture(OpenGLTexture const&) = default;
	OpenGLTexture(OpenGLTexture&&) = default;
	virtual ~OpenGLTexture() = default;

	OpenGLTexture& operator=(OpenGLTexture const&) = default;
	OpenGLTexture& operator=(OpenGLTexture&&) = default;

	static GLenum type() { return Type; };
	static Shader& shader(Window& window) { return window.getShader("texture"); }


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
	OpenGLTexture& setGeometry(float width, float height) {
		m_textureReference->setGeometry(width, height);

		return *this;
	}
	OpenGLTexture& setGeometry(float width, float height, float aspectRatio) {
		m_textureReference->setGeometry(width, height, aspectRatio);

		return *this;
	};
	OpenGLTexture& setPremultiplied(bool premultiplied = true) {
		m_textureReference->setPremultiplied(premultiplied);

		return *this;
	}

	operator TextureReferencePtr() const { return m_textureReference; }

protected:
	virtual void update() {
	}
	void onChange(ChangeNotifier&) override {
		update();
	}

private:
	TextureReferencePtr m_textureReference;
};


/** @short A RAII wrapper for binding to a texture (using it, modifying it) **/
class TextureBinder {
public:
	TextureBinder(const TextureBinder&) = delete;
	const TextureBinder& operator=(const TextureBinder&) = delete;
	/// constructor
	template <GLenum Type> TextureBinder(Window& window, OpenGLTexture<Type> const& tex) :
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

	m_textureReference->addChangeReceiver(this);
}

template <GLenum Type> OpenGLTexture<Type>::OpenGLTexture(TextureReferencePtr texture)
	: m_textureReference(texture) {

	m_textureReference->addChangeReceiver(this);
}

template <GLenum Type> void OpenGLTexture<Type>::draw(Window& window, Dimensions const& dim, TexCoords const& tex) const {
	glutil::GLErrorChecker glerror("OpenGLTexture::draw()");

	if (dim.getAngle() != 0.f) {
		auto matrix = glm::mat3(1.f);

		matrix = glm::scale(matrix, glmath::vec2(0.33f));
		matrix = glm::translate(matrix, glmath::vec2(dim.xc(), dim.yc()));
		matrix = glm::rotate(matrix, dim.getAngle());
		matrix = glm::translate(matrix, glmath::vec2(-dim.xc(), -dim.yc()));

		draw(window, dim, tex, matrix);

		return;
	}

	auto binder = TextureBinder(window, *this);

	glerror.check("texture");

	// The texture wraps over at the edges (repeat)
	const bool repeating = tex.outOfBounds();
	glTexParameterf(type(), GL_TEXTURE_WRAP_S, repeating ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameterf(type(), GL_TEXTURE_WRAP_T, repeating ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glerror.check("repeat mode");

	glutil::VertexArray va;
	va.texCoord(tex.x1, tex.y1).vertex(dim.x1(), dim.y1());
	va.texCoord(tex.x2, tex.y1).vertex(dim.x2(), dim.y1());
	va.texCoord(tex.x1, tex.y2).vertex(dim.x1(), dim.y2());
	va.texCoord(tex.x2, tex.y2).vertex(dim.x2(), dim.y2());

	va.draw();
}

template <GLenum Type> void OpenGLTexture<Type>::draw(Window& window, Dimensions const& dim, TexCoords const& tex, glmath::mat3 const& matrix) const {
	glutil::GLErrorChecker glerror("OpenGLTexture::draw()");
	glutil::VertexArray va;

	auto binder = TextureBinder(window, *this);

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
