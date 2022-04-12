#pragma once

#include "control.hh"

#include <set>

class UserControl : public Control {
  public:

	void addControl(Control* child);
	void addControl(Control& child);
	void removeControl(Control* child);
	void removeControl(Control& child);
	std::set<Control*> getChildren() const;

	void draw(GraphicContext&) override;

  private:
	  std::set<Control*> m_children;
};

