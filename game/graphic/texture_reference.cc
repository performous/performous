#include "texture_reference.hh"

TextureReference::TextureReference()
{
	glGenTextures(1, &m_id);
}

TextureReference::~TextureReference()
{
	glDeleteTextures(1, &m_id);
}

TextureReference::operator GLuint() const
{
	return m_id;
}

GLuint TextureReference::getId() const
{
	return m_id;
}

float TextureReference::getWidth() const
{
	return m_width;
}

float TextureReference::getHeight() const
{
	return m_height;
}

TextureReference& TextureReference::setGeometry(float width, float height)
{
	m_width = width;
	m_height = height;

	return *this;
}

TextureReference& TextureReference::setGeometry(float width, float height, float aspectRatio)
{
	m_width = width;
	m_height = height;
	m_aspectRatio = aspectRatio;

	return *this;
}

float TextureReference::getAspectRatio() const
{
	return m_aspectRatio;
}

bool TextureReference::isPremultiplied() const
{
	return m_premultiplied;
}

TextureReference& TextureReference::setPremultiplied(bool premultiplied)
{
	m_premultiplied = premultiplied;

	return *this;
}
