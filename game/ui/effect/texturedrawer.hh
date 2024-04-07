#pragma once

#include "ui/effect/ieffect.hh"
#include "graphic/texture.hh"

#include <optional>

class TextureDrawer : public IEffect {
public:
	TextureDrawer(Texture const&);

	void process(EffectContext&) override;

	void setColor(Color const&);
	void resetColor();
	void setMatrix(glm::mat3 const&);

private:
	Texture const& m_texture;
	std::optional<Color> m_color;
	glm::mat3 m_matrix{1.f};
};

