#include "Size.hh"

Size::Size(float width, float height)
: width(width), height(height) {
}

Size Size::operator*(float f) const {
	return {width * f, height * f};
}

Size Size::operator/(float d) const {
	return {width / d, height / d};
}

float Size::getWidth() const {
	return width;
}

float Size::getHeight() const {
	return height;
}



