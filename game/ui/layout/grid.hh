#pragma once

#include "ui/usercontrol.hh"
#include "ui/layout/ilayoutable.hh"

#include <vector>

class Grid : public UserControl, public ILayoutable {
  public:
	Grid(unsigned columns, unsigned rows);

	unsigned countColumns() const;
	unsigned countRows() const;

	Control const* operator()(unsigned column, unsigned row) const;
	Control*& operator()(unsigned column, unsigned row);
	Control const* getControl(unsigned column, unsigned row) const;
	Control*& getControl(unsigned column, unsigned row);

	void layout() override;

	std::set<Control*> getChildren() const override;

	void resizeColumns(std::vector<float> const&);
	void resizeRows(std::vector<float> const&);

	void draw(GraphicContext& gc) override;

  private:
	unsigned m_columns;
	unsigned m_rows;
	std::vector<Control*> m_controls;
	std::vector<float> m_columnWidths;
	std::vector<float> m_rowHeights;
};
