#include "panel.hh"

Panel::Panel() {
}

void Panel::layout() {
	auto dx = getX() - m_x;
	auto dy = getY() - m_y;

	for(auto child : getChildren()) {
		auto const x = child->getX() + dx;
		auto const y = child->getY() + dy;

		child->setGeometry(x, y, child->getWidth(), child->getHeight());
	}
}

