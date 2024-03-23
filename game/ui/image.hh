#pragma once

#include "control.hh"
#include "graphic/texture.hh"
#include "imagemodifier/iimagemodifier.hh"

#include <memory>

class Image : public Control {
  public:
	Image(std::string const& texture = {}, Control* parent = nullptr);
	Image(Control* parent, std::string const& texture = {});

	void setTexture(std::string const& texture);

	bool canFocused() const override { return m_canBeFocused; }
	void canBeFocused(bool value) { m_canBeFocused = value; }
	void draw(GraphicContext&) override;

	void setModifier(ImageModifier);

	void initialize(Game&) override;

	Image& setBackgroundDrawing(bool draw);
	bool getBackgroundDrawing() const;

  private:
	Texture m_texture;
	Texture m_background;
	bool m_drawBackground = true;
	std::string m_path;
	bool m_canBeFocused = false;
	ImageModifier m_modifier;
};
