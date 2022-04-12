#pragma once

#include "control.hh"
#include "text.hh"
#include "../texture.hh"

#include <tuple>
#include <vector>

class Item {
  public:
	Item(std::string const& text);
	Item(std::string const& id, std::string const& text);

	std::string getId() const;
	std::string toString() const;

  private:
	std::string m_id;
	std::string m_text;
};

class List : public Control {
  public:
	  const static size_t None;

  public:
	List(std::vector<Item> const& items = {}, Control* parent = nullptr);
	List(Control* parent, std::vector<Item> const& items = {});

	void setItems(std::vector<Item> const&);
	std::vector<Item> getItems() const;
	size_t countItems() const;

	Item const& getSelected() const;
	size_t getSelectedIndex() const;

	void select(size_t);
	void select(std::string const& id);

	void onKey(Key) override;

	void draw(GraphicContext&) override;

  private:
	void updateTexts();

  private:
	Texture m_background;
	Texture m_selectedBackground;
	std::vector<Item> m_items;
	std::vector<Text> m_texts;
	size_t m_selected = -1;
};


