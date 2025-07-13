#pragma once

#include "ui/path/ipath.hh"

#include <memory>

class BorderPath : public IPath {
  public:
	BorderPath(float radius = 0.f);

	void setGeometry(float x, float y, float width, float height);
	float getX() const;
	float getY() const;
	float getWidth() const;
	float getHeight() const;

	float getLength() const override;
	Point getPoint(float) const override;

  private:
	float m_x = 0.f;
	float m_y = 0.f;
	float m_width = 0.f;
	float m_height = 0.f;
	float m_radius = 0.f;
};

using BorderPathPtr = std::shared_ptr<BorderPath>;
