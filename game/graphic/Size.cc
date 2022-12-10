#include "Size.hh"

Size::Size(float width, float height)
: width(width), height(height) {
}

float Size::getWidth() const {
	return width;
}

float Size::getHeight() const {
	return height;
}



