#include "screen.hh"

#include "game.hh"

void Screen::drawImages(Theme& theme) {
	for (auto& image : theme.images) {
		image.texture->dimensions.middle(image.x).center(image.y).scale(image.scale);
		image.texture->draw(getGame().getWindow());
	}
}
