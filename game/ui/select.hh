#pragma once

#include "control.hh"
#include "text.hh"
#include "../texture.hh"

#include <vector>

class Select : public Control {
  public:
	Select(std::vector<std::string> const& items = {}, Control* parent = nullptr);
	Select(Control* parent, std::vector<std::string> const& items = {});

	void setItems(std::vector<std::string> const&);
	std::vector<std::string> getItems() const;
	size_t countItems() const;

	std::string getSelectedText() const;
	size_t getSelectedIndex() const;

	void select(size_t);

	void onKey(Key) override;

	void draw(GraphicContext&) override;

  private:
	Text m_text;
	std::unique_ptr<Texture> m_background;
	std::unique_ptr<Texture> m_up_down;
	std::vector<std::string> m_items;
	size_t m_selected = size_t(-1);
};

