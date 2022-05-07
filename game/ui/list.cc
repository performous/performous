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

void Item::setText(const std::string& text) {
	m_text = text;
}

std::string Item::getIcon() const {
	return m_icon;
}

void Item::setIcon(const std::string& icon) {
	m_icon = icon;
}

bool Item::isChecked() const {
	return m_checked;
}

void Item::setChecked(bool checked) {
	m_checked = checked;
}

/*
std::any const& Item::getUserData() const {
	return m_userData;
}
*/
void Item::setUserData(std::any const& data) {
	m_userData = data;
}


const size_t List::None = -1;

List::List(std::vector<Item> const& items, Control* parent)
: Control(parent), m_background(findFile("mainmenu_back_highlight.svg")), m_selectedBackground(findFile("mainmenu_back_highlight.svg")), m_items(items) {
}

List::List(Control* parent, std::vector<Item> const& items)
: Control(parent), m_background(findFile("mainmenu_back_highlight.svg")), m_selectedBackground(findFile("mainmenu_back_highlight.svg")), m_items(items) {
}

void List::setItems(std::vector<Item> const& items) {
	m_items = items;

	if(m_selected == None && countItems() > 0)
		select(0);
	if(m_selected != None && m_selected >= countItems())
		select(countItems() - 1);
}

std::vector<Item> const& List::getItems() const {
	return m_items;
}

size_t List::countItems() const {
	return m_items.size();
}

Item const& List::getSelected() const {
	return m_items.at(m_selected);
}

Item& List::getSelected() {
	return m_items.at(m_selected);
}

size_t List::getSelectedIndex() const {
	return m_selected;
}

void List::select(size_t index) {
	if(m_selected != index) {
		if(m_onSelectionChanged)
			m_onSelectionChanged(*this, index, m_selected);

		m_selected = index;
	}
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

void List::onSelectionChanged(const std::function<void(List&, size_t, size_t)>& callback) {
	m_onSelectionChanged = callback;
}

void List::displayIcon(bool display) {
	m_displayIcon = display;
}

bool List::isDisplayingIcon() const {
	return m_displayIcon;
}

void List::displayCheckBox(bool display) {
	m_displayCheckBox = display;
}

bool List::isDisplayingCheckBox() const {
	return m_displayCheckBox;
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
		case Key::Space:
			if(isDisplayingCheckBox() && m_selected != None)
				m_checkBoxs[m_selected]->toggleState();
			break;
		default:;
	}
}

void List::updateTexts() {
	m_texts.resize(m_items.size());

	for(auto i = 0U; i < m_items.size(); ++i)
		m_texts[i].setText(m_items[i].toString());
}

void List::updateIcons() {
	if(m_displayIcon) {
		m_icons.resize(m_items.size());

		for(auto i = 0U; i < m_items.size(); ++i) {
			auto const path = m_items[i].getIcon();

			if(!m_icons[i])
				m_icons[i] = std::make_unique<Image>(path, this);
			else
				m_icons[i]->setTexture(path);
		}
	}
}

void List::updateCheckBoxs() {
	if(m_displayCheckBox) {
		m_checkBoxs.resize(m_items.size());

		for(auto i = 0U; i < m_items.size(); ++i) {
			if(!m_checkBoxs[i]) {
				m_checkBoxs[i] = std::make_unique<CheckBox>(this, m_items[i].isChecked());
				m_checkBoxs[i]->onStateChanged
					([&, i](CheckBox&, bool checked){
						m_items[i].setChecked(checked);
						std::cout << "set item " << i << " to " << (checked ? "checked" : "unchecked") << std::endl;
						std::cout << "set item " << i << " to " << (m_items[i].isChecked() ? "checked" : "unchecked") << std::endl;
					});
			}
		}
	}
}

void List::draw(GraphicContext& gc) {
	drawFocus();

	m_background.dimensions.left(getX()).top(getY()).stretch(getWidth(), getHeight());
	m_background.draw();

	if(m_items.size() != m_texts.size())
		updateTexts();
	updateIcons();
	updateCheckBoxs();

	if(m_items.empty())
		return;

	const auto lineHeight = 0.025f;
	const auto itemsToDisplay = std::min(countItems(), size_t((getHeight() - lineHeight) / lineHeight));
	const auto itemsToDisplayHalf = itemsToDisplay / 2;
	const auto itemToStart = getSelectedIndex() == None ? 0 :
		std::min(int(countItems()) - int(itemsToDisplay), std::max(0, int(getSelectedIndex()) - int(itemsToDisplayHalf)));
	auto y = getY() + lineHeight * 0.5f;

	for(auto i = size_t(0); i < itemsToDisplay; ++i) {
		const auto index = size_t(itemToStart) + i;

		if(index == getSelectedIndex()) {
			m_selectedBackground.dimensions.left(getX()).top(y).stretch(getWidth(), lineHeight);
			m_selectedBackground.draw();
		}

		const auto color = ColorTrans(Color::alpha(index == getSelectedIndex() ? 1.0 : 0.5));

		auto x = getX();
		auto width = getWidth();

		if(m_displayIcon) {
			m_icons[index]->setGeometry(x, y, lineHeight, lineHeight);
			m_icons[index]->draw(gc);

			x += lineHeight;
			width -= lineHeight;
		}
		if(m_displayCheckBox)
			width -= lineHeight;

		gc.draw(m_texts[index], x, y, width, lineHeight);

		x += width;

		if(m_displayCheckBox) {
			m_checkBoxs[index]->setGeometry(x, y, lineHeight, lineHeight);
			m_checkBoxs[index]->draw(gc);
		}

		y += lineHeight;
	}
}


