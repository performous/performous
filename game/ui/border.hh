#pragma once

#include "texture.hh"
#include "ui/path/ipathprovider.hh"
#include "ui/path/borderpath.hh"

#include <memory>
#include <string>

class BorderDefinition : public Texture {
  public:
	BorderDefinition(std::string const& texture);
	BorderDefinition(std::string const& texture, float borderWidthInPixel);

	float getBorderWidth() const;

  private:
	float m_borderWidth = 0.2f;
};

using BorderDefinitionPtr = std::shared_ptr<BorderDefinition>;

class Border : public IPathProvider {
  public:
	Border(BorderDefinitionPtr);
	Border(BorderDefinitionPtr, float x, float y, float width, float height);

	void setGeometry(float x, float y, float width, float height);
	float getX() const;
	float getY() const;
	float getWidth() const;
	float getHeight() const;

	void draw(Window&);

	PathPtr getPath() const override;

  private:
	  void update();

  private:
	BorderDefinitionPtr m_definition;
	float m_x = 0;
	float m_y = 0;
	float m_width = 0;
	float m_height = 0;
	float m_border = 0.005f;
	BorderPathPtr m_path{ std::make_shared<BorderPath>() };
};
