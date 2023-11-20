#include "screen.hh"

#include "game.hh"
#include "graphic/color_trans.hh"
#include <SDL_timer.h>

void Screen::drawImages(Theme const& theme) {
	drawImages(theme.images);
}

void Screen::drawImages(std::vector<Theme::Image> const& images) {
	for (auto& image : images) {
		ColorTrans c(getGame().getWindow(), Color::alpha(image.alpha));

		image.texture->dimensions.middle(image.x).center(image.y).scale(image.scaleHorizontal, image.scaleVertical);
		image.texture->draw(getGame().getWindow());
	}
}

void Screen::setTheme(std::shared_ptr<Theme> theme) {
	m_theme = theme;
}

std::shared_ptr<Theme> Screen::drawTheme() {
	return m_theme;
}

void Screen::setBackground(std::shared_ptr<Texture> image) {
	m_background = image;
}

namespace {
	using ColorModifier = std::vector<std::unique_ptr<ColorTrans>>;

	ColorModifier makeColorModifier(Theme const& theme, float alpha, Window& window) {
		auto result = ColorModifier{};

		if (theme.colorcycling) {
			auto const cycleDurationMS = theme.colorcycleduration * 1000;
			auto anim = static_cast<float>(SDL_GetTicks() % cycleDurationMS) / float(cycleDurationMS);

			result.emplace_back(std::make_unique<ColorTrans>(window, glmath::rotate(static_cast<float>(TAU * anim), glmath::vec3(1.0f, 1.0f, 1.0f))));
		}
		if(alpha != 1.f)
			result.emplace_back(std::make_unique<ColorTrans>(window, Color::alpha(alpha)));

		return result;
	}
}

void Screen::drawBackground() {
	auto& window = getGame().getWindow();

	if (m_background) {
		if (m_theme) {
			auto const* image = findImage("background", *m_theme);
			auto alpha = 1.0f;

			if (image)
				alpha = image->alpha;

			auto const colorModifier = makeColorModifier(*m_theme, alpha, window);

			m_background->draw(window);
		}
		else {
			m_background->draw(window);
		}
	}
}

Theme::Image* Screen::findImage(std::string const& id, Theme& theme) {
	for (auto& image : theme.images)
		if (image.id == id)
			return &image;

	return getGame().findImage(id);
}
