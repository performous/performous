#include "form.hh"

Form::Form(Game& game)
: m_game(game) {
}

void Form::focus(Control const& control) {
	auto children = collectChildren([](Control const& control){return control.canFocused();});

	if(children.empty())
		return;

	if(m_focus == &control)
		return;

	if(m_focus)
		m_focus->setFocus(false);

	auto it = std::find(children.begin(), children.end(), &control);

	if(it != children.end()) {
		m_focus = *it;
		m_focus->setFocus(true);
	}
}

void Form::focusNext() {
	auto childSet = collectChildren([](Control const& control){return control.canFocused();});

	if(childSet.empty())
		return;

	auto children = std::vector<Control*>(childSet.begin(), childSet.end());

	std::sort(children.begin(), children.end(), [](Control const* a, Control const* b){ return a->getTabIndex() < b->getTabIndex();});

	if(!m_focus)
		m_focus = children.front();
	else {
		auto it = std::find(children.begin(), children.end(), m_focus);

		m_focus->setFocus(false);

		if(it == children.end() || ++it == children.end())
			m_focus = *children.begin();
		else
			m_focus = *it;
	}

	if(m_focus->canFocused())
		m_focus->setFocus(true);
	else
		focusNext();
}

void Form::focusPrevious() {
	auto childSet = collectChildren([](Control const& control){return control.canFocused();});

	if(childSet.empty())
		return;

	auto children = std::vector<Control*>(childSet.begin(), childSet.end());

	std::sort(children.begin(), children.end(), [](Control const* a, Control const* b){ return a->getTabIndex() < b->getTabIndex();});

	if(!m_focus)
		m_focus = *children.rbegin();
	else {
		auto it = std::find(children.begin(), children.end(), m_focus);

		m_focus->setFocus(false);

		if(it == children.begin())
			m_focus = *children.rbegin();
		else
			m_focus = *--it;
	}

	m_focus->setFocus(true);

	if(m_focus->canFocused())
		m_focus->setFocus(true);
	else
		focusPrevious();
}

void Form::onKey(Key key) {
	switch(key) {
		case Key::BackTab:
			focusPrevious();
			break;
		case Key::Tab:
			focusNext();
			break;
		default:
			if(m_focus)
				m_focus->onKey(key);
	}
}

void Form::onKeyDown(Key key) {
	switch(key) {
		case Key::BackTab:
			break;
		case Key::Tab:
			break;
		default:
			if(m_focus)
				m_focus->sendOnKeyDown(key);
	}
}

void Form::onKeyUp(Key key) {
	switch(key) {
		case Key::BackTab:
			break;
		case Key::Tab:
			break;
		default:
			if(m_focus)
				m_focus->sendOnKeyUp(key);
	}
}

Game& Form::getGame() {
	return m_game;
}

