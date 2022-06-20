#include "point.hh"

Point::Point(float x, float y)
	: m_x(x), m_y(y)
{
}

float Point::getX() const {
	return m_x;
}

float Point::getY() const {
	return m_y;
}

Point& Point::setX(float x) {
	m_x = x;

	return *this;
}

Point& Point::setY(float y) {
	m_y = y;

	return *this;
}

Point Point::operator+(Point const& other) const {
	return Point(m_x + other.m_x, m_y + other.m_y);
}

Point Point::operator-(Point const& other) const {
	return Point(m_x - other.m_x, m_y - other.m_y);
}

std::ostream& operator<<(std::ostream& os, Point const& point) {
	return os << point.getX() << ":" << point.getY();
}
