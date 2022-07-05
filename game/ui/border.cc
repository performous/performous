#include "border.hh"
#include <glutil.hh>
#include <texture.hh>
#include <util.hh>
#include <game.hh>
#include <video_driver.hh>

BorderDefinition::BorderDefinition(const std::string& texture)
	: Texture(texture) {
}

BorderDefinition::BorderDefinition(const std::string& texture, float borderWidthInPixel)
	: Texture(texture), m_borderWidth(borderWidthInPixel) {
}

float BorderDefinition::getBorderWidth() const {
	return m_borderWidth;
}

Border::Border(BorderDefinitionPtr definition)
	: m_definition(definition) {
}

Border::Border(BorderDefinitionPtr definition, float x, float y, float width, float height)
	: m_definition(definition), m_x(x), m_y(y), m_width(width), m_height(height) {
}

void Border::setGeometry(float x, float y, float width, float height) {
	m_x = x;
	m_y = y;
	m_width = width;
	m_height = height;

	update();
}

float Border::getX() const {
	return m_x;
}

float Border::getY() const {
	return m_y;
}

float Border::getWidth() const {
	return m_width;
}

float Border::getHeight() const {
	return m_height;
}

void Border::draw(Window& window) {
	glutil::GLErrorChecker glerror("Border::draw()");

	auto const texture = UseTexture(window, *m_definition);
	glerror.check("texture");

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glerror.check("repeat mode");

	auto const dim = Dimensions().left(getX()).top(getY()).stretch(getWidth(), getHeight());
	auto const left = m_definition->getBorderWidth();
	auto const right = 1.0 - left;
	auto const top = m_definition->getBorderWidth();
	auto const bottom = 1.0 - top;
	auto const border = m_border;
	auto const x1 = dim.x1() - border;
	auto const x2 = dim.x2() + border;
	auto const xl = dim.x1();
	auto const xr = dim.x2();
	auto const y1 = dim.y1() - border;
	auto const y2 = dim.y2() + border;
	auto const yt = dim.y1();
	auto const yb = dim.y2();

	glutil::VertexArray va0;
	va0.texCoord(0, 0).vertex(x1, y1);
	va0.texCoord(0, top).vertex(x1, yt);
	va0.texCoord(left, 0).vertex(xl, y1);
	va0.texCoord(left, top).vertex(xl, yt);
	va0.texCoord(right, 0).vertex(xr, y1);
	va0.texCoord(right, top).vertex(xr, yt);
	va0.texCoord(1, 0).vertex(x2, y1);
	va0.texCoord(1, top).vertex(x2, yt);
	va0.draw();

	glutil::VertexArray va1;
	va1.texCoord(0, top).vertex(x1, yt);
	va1.texCoord(0, bottom).vertex(x1, yb);
	va1.texCoord(left, top).vertex(xl, yt);
	va1.texCoord(left, bottom).vertex(xl, yb);
	va1.texCoord(right, top).vertex(xr, yt);
	va1.texCoord(right, bottom).vertex(xr, yb);
	va1.texCoord(1, top).vertex(x2, yt);
	va1.texCoord(1, bottom).vertex(x2, yb);
	va1.draw();

	glutil::VertexArray va2;
	va2.texCoord(0, bottom).vertex(x1, yb);
	va2.texCoord(0, 1).vertex(x1, y2);
	va2.texCoord(left, bottom).vertex(xl, yb);
	va2.texCoord(left, 1).vertex(xl, y2);
	va2.texCoord(right, bottom).vertex(xr, yb);
	va2.texCoord(right, 1).vertex(xr, y2);
	va2.texCoord(1, bottom).vertex(x2, yb);
	va2.texCoord(1, 1).vertex(x2, y2);
	va2.draw();

}

PathPtr Border::getPath() const {
	return m_path;
}

void Border::update() {
	auto const halfBorder = m_border * 0.5f;

	m_path->setGeometry(m_x - halfBorder, m_y - halfBorder, m_width + m_border, m_height + m_border);
}
