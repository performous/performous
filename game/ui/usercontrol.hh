#pragma once

#include "control.hh"

#include <set>

class UserControl : public Control {
  public:
	void addControl(Control* child);
	void addControl(Control& child);
	void removeControl(Control* child);
	void removeControl(Control& child);
	virtual std::set<Control*> getChildren() const;
	virtual std::set<Control*> collectChildren(std::function<bool(Control const&)> const&) const;

	void draw(GraphicContext&) override;

  private:
	std::set<Control*> m_children;
};

