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

Theme::Image* Screen::findImage(std::string const& id, Theme& theme) {
	for (auto& image : theme.images)
		if (image.id == id)
			return &image;

	return getGame().findImage(id);
}
