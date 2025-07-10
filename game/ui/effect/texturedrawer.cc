#include "ui/effect/texturedrawer.hh"
#include "ui/effect/effectcontext.hh"
#include "ui/graphiccontext.hh"
#include "graphic/color_trans.hh"

#include <glm/gtx/matrix_transform_2d.hpp>

#include <memory>
#include <iostream>

TextureDrawer::TextureDrawer(const Texture& texture)
: m_texture(texture) {
}

void TextureDrawer::process(EffectContext& effectContext) {
	auto& window = effectContext.getGraphicContext().getWindow();
	auto const color = m_color ? std::make_unique<ColorTrans>(window, m_color.value()) : std::unique_ptr<ColorTrans>();

	//m_texture.draw(glm::scale(m_matrix, glm::vec2(3.f)));
	m_texture.draw(window, m_matrix);
}

void TextureDrawer::setColor(const Color& color) {
	m_color = color;
}

void TextureDrawer::resetColor() {
	m_color.reset();
}

void TextureDrawer::setMatrix(const glm::mat3& matrix) {
   //std::cout << "matrix" << std::endl;
	m_matrix = matrix;
}
