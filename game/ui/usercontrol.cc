#include "usercontrol.hh"

void UserControl::addControl(Control* child) {
	m_children.insert(child);

	child->setParent(this);
}

void UserControl::addControl(Control& child) {
	m_children.insert(&child);

	child.setParent(this);
}

void UserControl::removeControl(Control* child) {
	m_children.erase(child);

	child->setParent(nullptr);
}

void UserControl::removeControl(Control& child) {
	m_children.erase(&child);

	child.setParent(nullptr);
}

std::set<Control*> UserControl::getChildren() const {
	return m_children;
}

void UserControl::draw(GraphicContext& gc) {
	for(auto* child : m_children)
		child->draw(gc);
}
