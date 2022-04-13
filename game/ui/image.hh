#pragma once

#include "control.hh"
#include <texture.hh>
#include <memory>

class Image : public Control {
  public:
	Image(std::string const& texture = {}, Control* parent = nullptr);
	Image(Control* parent, std::string const& texture = {});

	void setTexture(std::string const& texture);

	bool canFocused() const override { return false; }
	void draw(GraphicContext&) override;

  private:
	std::unique_ptr<Texture> m_texture;
	Texture m_background;
};
