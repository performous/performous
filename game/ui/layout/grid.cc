#include "grid.hh"

Grid::Grid(unsigned columns, unsigned rows)
: m_columns(columns), m_rows(rows) {
	m_controls.resize(columns * rows);
	m_columnWidths.resize(columns, 1.0f / float(columns));
	m_rowHeights.resize(rows, 1.0f / float(rows));
}

unsigned Grid::countColumns() const {
	return m_columns;
}

unsigned Grid::countRows() const {
	return m_rows;
}

Control const* Grid::operator()(unsigned column, unsigned row) const {
	return m_controls.at(column + row * m_columns);
}

Control const* Grid::getControl(unsigned column, unsigned row) const {
	return m_controls.at(column + row * m_columns);
}

Control* Grid::getControl(unsigned column, unsigned row) {
	return m_controls.at(column + row * m_columns);
}

void Grid::setControl(unsigned column, unsigned row, Control* child) {
	auto oldChild = getControl(column, row);

	if (oldChild && oldChild->getParent() == this)
		oldChild->setParent(nullptr);

	m_controls.at(column + row * m_columns) = child;

	if (child)
		child->setParent(this);
}

void Grid::layout() {
	auto x = getX();

	for(auto column = 0U; column < m_columns; ++column) {
		auto y = getY();

		for(auto row = 0U; row < m_rows; ++row) {
			auto control = getControl(column, row);

			if(control) {
				auto const w = m_columnWidths[column] * getWidth();
				auto const h = m_rowHeights[row] * getHeight();

				std::cout << x << ", " << y << ", " << w << ", " << h << std::endl;

				control->setGeometry(x, y, w ,h);

				auto layoutable = dynamic_cast<ILayoutable*>(control);

				if(layoutable)
					layoutable->layout();
			}

			y += m_rowHeights[row];
		}

		x += m_columnWidths[column];
	}
}

std::set<Control*> Grid::getChildren() const {
	auto children = std::set<Control*>();

	for(auto child : m_controls) {
		if(child)
			children.insert(child);
	}

	return children;
}

void Grid::resizeColumns(std::vector<float> const& sizes) {
	m_columnWidths = sizes;
}

void Grid::resizeRows(std::vector<float> const& sizes) {
	m_rowHeights = sizes;
}

void Grid::draw(GraphicContext& gc) {
	for(auto* child : m_controls)
		if(child)
			child->draw(gc);
}

void Grid::initialize(Game& game) {
	for (auto* child : m_controls)
		if (child)
			child->initialize(game);

	UserControl::initialize(game);
}
