#include "checkbox.hh"
#include "graphiccontext.hh"

CheckBox::CheckBox(bool checked, Control* parent)
: Control(parent), m_checkedImage(findFile("checkbox_checked.svg")), m_uncheckedImage(findFile("checkbox_unchecked.svg")), m_checked(checked) {
}

CheckBox::CheckBox(Control* parent, bool checked)
: CheckBox(checked, parent) {
}

bool CheckBox::isChecked() const {
	return m_checked;
}

void CheckBox::setChecked(bool checked) {
	if(m_checked != checked) {
		m_checked = checked;
		if(m_onStateChanged)
			m_onStateChanged(*this, m_checked);
	}
}

bool CheckBox::getState() const {
	return m_checked;
}

void CheckBox::toggleState() {
	setChecked(!isChecked());
}

void CheckBox::onStateChanged(const std::function<void (CheckBox &, bool)>& callback) {
	m_onStateChanged = callback;
}

void CheckBox::onKey(Key key) {
	if(key == Key::Space)
		setChecked(!isChecked());
}

void CheckBox::draw(GraphicContext& gc) {
	if(m_checked) {
		m_checkedImage.dimensions.left(getX()).top(getY()).stretch(getWidth(), getHeight());
		m_checkedImage.draw(gc.getWindow());
		m_uncheckedImage.dimensions.left(getX()).top(getY()).stretch(0, 0);
		m_uncheckedImage.draw(gc.getWindow());
	}
	else {
		m_uncheckedImage.dimensions.left(getX()).top(getY()).stretch(getWidth(), getHeight());
		m_uncheckedImage.draw(gc.getWindow());
		m_checkedImage.dimensions.left(getX()).top(getY()).stretch(0, 0);
		m_checkedImage.draw(gc.getWindow());
	}
}


