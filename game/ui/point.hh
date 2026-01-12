#pragma once

#include <iostream>

class Point {
public:
	Point() = default;
	Point(float x, float y);

	float getX() const;
	float getY() const;
	Point& setX(float);
	Point& setY(float);

	Point operator+(Point const&) const;
	Point operator-(Point const&) const;

private:
	float m_x = 0.f;
	float m_y = 0.f;
};

std::ostream& operator<<(std::ostream&, Point const&);
