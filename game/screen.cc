#include "screen.hh"

#include "game.hh"

void Screen::drawImages(Theme const& theme) {
	drawImages(theme.images);
}

void Screen::drawImages(std::vector<Theme::Image> const& images) {
	for (auto& image : images) {
		image.texture->dimensions.middle(image.x).center(image.y).scale(image.scale);
		image.texture->draw(getGame().getWindow());
	}
}
