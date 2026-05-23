#pragma once

#include "ui/usercontrol.hh"
#include "ui/layout/ilayoutable.hh"

#include <vector>

class Panel : public UserControl, public ILayoutable {
  public:
	Panel();

	void layout() override;

  private:
	float m_x = 0.f;
	float m_y = 0.f;
};

