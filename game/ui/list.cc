#include "list.hh"
#include "graphiccontext.hh"

Item::Item(std::string const& text)
: m_id(text), m_text(text) {
}

Item::Item(std::string const& id, std::string const& text)
: m_id(id), m_text(text) {
}

std::string Item::getId() const {
	return m_id;
}

std::string Item::toString() const {
	return m_text;
}

const size_t List::None = -1;

List::List(std::vector<Item> const& items, Control* parent)
: Control(parent), m_background(findFile("mainmenu_comment_bg.svg")), m_selectedBackground(findFile("mainmenu_back_highlight.svg")), m_items(items) {
}

List::List(Control* parent, std::vector<Item> const& items)
: Control(parent), m_background(findFile("mainmenu_comment_bg.svg")), m_selectedBackground(findFile("mainmenu_back_highlight.svg")), m_items(items) {
}

void List::setItems(std::vector<Item> const& items) {
	m_items = items;

	if(m_selected == None && countItems() > 0)
		select(0);
	if(m_selected != None && m_selected >= countItems())
		select(countItems() - 1);
}

std::vector<Item> List::getItems() const {
	return m_items;
}

size_t List::countItems() const {
	return m_items.size();
}

Item const& List::getSelected() const {
	return m_items.at(m_selected);
}

size_t List::getSelectedIndex() const {
	return m_selected;
}

void List::select(size_t index) {
	m_selected = index;
}

void List::select(const std::string& id) {
	for(auto i = 0U; i < m_items.size(); ++i) {
		if(id == m_items[i].getId()) {
			select(i);
			return;
		}
	}

	throw std::runtime_error("No item with id '" + id + "' found!");
}

void List::onKey(Key key) {
	if(countItems() == 0)
		return;

	switch(key) {
		case Key::Up:
			select(std::max(int(m_selected) - 1, 0));
			break;
		case Key::Down:
			select(std::min(m_selected + 1, countItems() - 1));
			break;
		default:;
	}
}

void List::updateTexts() {
	m_texts.resize(m_items.size());

	for(auto i = 0U; i < m_items.size(); ++i)
		m_texts[i].setText(m_items[i].toString());
}

void List::draw(GraphicContext& gc) {
	m_background.dimensions.left(getX()).top(getY()).stretch(getWidth(), getHeight());
	m_background.draw();

	if(m_items.size() != m_texts.size())
		updateTexts();

	const auto lineHeight = 0.025;
	const auto itemsToDisplay = size_t((getHeight() - lineHeight) / lineHeight);
	const auto itemsToDisplayHalf = itemsToDisplay / 2;
	const auto itemToStart = getSelectedIndex() == None ? 0 :
		std::min(int(countItems()) - int(itemsToDisplay), std::max(0, int(getSelectedIndex()) - int(itemsToDisplayHalf)));
	auto y = getY() + lineHeight * 0.5;

	for(auto i = 0U; i < itemsToDisplay; ++i) {
		const auto index = itemToStart + i;

		if(index == getSelectedIndex()) {
			m_selectedBackground.dimensions.left(getX()).top(y).stretch(getWidth(), lineHeight);
			m_selectedBackground.draw();
		}

		const auto color = ColorTrans(Color::alpha(index == getSelectedIndex() ? 1.0 : 0.5));

		gc.drawCentered(m_texts[index], getX(), y, getWidth(), lineHeight);

		y += lineHeight;
	}
}


