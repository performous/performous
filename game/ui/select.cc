#include "select.hh"
#include "graphiccontext.hh"
#include "graphic/color_trans.hh"

Select::Select(std::vector<std::string> const& items, Control* parent)
: Control(parent), m_text(""), m_items(items) {
}

Select::Select(Control* parent, std::vector<std::string> const& items)
: Control(parent), m_text(""), m_items(items) {
}

void Select::setItems(std::vector<std::string> const& items) {
	m_items = items;

	select(0);
}

std::vector<std::string> Select::getItems() const {
	return m_items;
}

size_t Select::countItems() const {
	return m_items.size();
}

std::string Select::getSelectedText() const {
	return m_items.at(m_selected);
}

size_t Select::getSelectedIndex() const {
	return m_selected;
}

void Select::select(size_t index) {
	m_selected = index;

	m_text.setText(getSelectedText());
}

void Select::onKey(Key key) {
	if(countItems() == 0)
		return;

	switch(key) {
		case Key::Up:
			select((m_selected + 1) % countItems());
			break;
		case Key::Down:
			select((countItems() + m_selected - 1) % countItems());
			break;
		default:;
	}
}

void Select::draw(GraphicContext& gc) {
	drawFocus(gc);

	const auto color = ColorTrans(gc.getWindow(), hasFocus() ? Color(1.f, 1.f, 1.f) : Color(0.6f, 0.6f, 0.6f));

	if (!m_background || !m_up_down) {
		m_background = gc.getTheme().getSelectBG();
		m_up_down = gc.getTheme().getSelectUpDown();
	}

	m_background->dimensions.left(getX()).top(getY()).stretch(getWidth(), getHeight());
	m_background->draw(gc.getWindow());

	auto const up_down_left = false;
	auto const width = getHeight() * 0.3f;
	auto const height = getHeight() * 0.6f;
	auto const x = up_down_left ? getHeight() * 0.1f : getWidth() - getHeight() * 0.1f - width;
	auto const y = (getHeight() - height) * 0.5f;

	m_up_down->dimensions.left(getX() + x).top(getY() + y).fixedHeight(height);
	m_up_down->draw(gc.getWindow());

	gc.drawCentered(m_text, getX(), getY(), getWidth(), getHeight());
}
