#include "form.hh"

void Form::focusNext() {
	if(getChildren().empty())
		return;

	if(!m_focus)
		m_focus = *getChildren().begin();
	else {
		auto children = getChildren();
		auto it = children.find(m_focus);

		m_focus->setFocus(false);

		if(it == children.end() || ++it == children.end())
			m_focus = *getChildren().begin();
		else
			m_focus = *it;
	}

	if(m_focus->canFocused())
		m_focus->setFocus(true);
	else
		focusNext();
}

void Form::focusPrevious() {
	if(getChildren().empty())
		return;

	if(!m_focus)
		m_focus = *getChildren().rbegin();
	else {
		auto children = getChildren();
		auto it = children.find(m_focus);

		m_focus->setFocus(false);

		if(it == children.begin() || --it == children.begin())
			m_focus = *getChildren().rbegin();
		else
			m_focus = *it;
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

