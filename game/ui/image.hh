#pragma once

#include "control.hh"
#include <texture.hh>
#include <memory>

class Image : public Control {
  public:
	Image(std::string const& texture = {}, Control* parent = nullptr);
	Image(Control* parent, std::string const& texture = {});

	void setTexture(std::string const& texture);

	bool canFocused() const override { return m_canBeFocused; }
	void canBeFocused(bool value) { m_canBeFocused = value; }
	void draw(GraphicContext&) override;

  private:
	std::unique_ptr<Texture> m_texture;
	std::unique_ptr<Texture> m_background;
	std::string m_path;
	bool m_canBeFocused = false;
};
