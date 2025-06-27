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

std::set<Control*> UserControl::collectChildren(std::function<bool(Control const&)> const& selector) const {
	auto result = std::set<Control*>();

	for(auto child : getChildren()) {
		if(!selector(*child))
			continue;

		auto usercontrol = dynamic_cast<UserControl*>(child);

		if(usercontrol) {
			auto children = usercontrol->collectChildren(selector);

			for(auto child : children)
				result.insert(child);
		}
		else
			result.insert(child);
	}

	return result;
}

void UserControl::draw(GraphicContext& gc) {
	for(auto* child : m_children)
		child->draw(gc);
}
