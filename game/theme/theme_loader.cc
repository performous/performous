#include "theme_loader.hh"
#include "json.hh"

namespace {
	ThemePtr createTheme(std::string const& screen) {
		if (screen == "Intro")
			return std::make_shared<ThemeIntro>();
		if (screen == "Songs")
			return std::make_shared<ThemeSongs>();
		if (screen == "Practice")
			return std::make_shared<ThemePractice>();

		throw std::logic_error("creation of theme for screen '" + screen + "' is not implemented!");
	}

	void loadTexture(Texture& texture, std::string const& filename) {
		try {
			auto const fullpath = findFile(filename);

			texture.load(fullpath);
		}
		catch (std::exception const& e) {
			std::clog << "caught exception while loading texture for theme: " << e.what() << std::endl;
		}
	}
}

ThemePtr ThemeLoader::load(std::string const& screenName)
{
	auto theme = createTheme(screenName);

	try {
		auto const fullpath = findFile("theme.json");
		auto const config = readJSON(fullpath);

		if (config.contains(screenName)) {
			auto const screenConfig = config.at(screenName);

			if (screenConfig.contains("background")) {
				auto const backgroundConfig = screenConfig.at("background");

				if (backgroundConfig.contains("image")) {
					auto const filename = backgroundConfig.at("image").get<std::string>();

					loadTexture(*theme->bg, filename);
				}
				if (backgroundConfig.contains("colorcycling")) {
					auto const colorcycling = backgroundConfig.at("colorcycling").get<bool>();

					theme->colorcycling = colorcycling;
				}
				if (backgroundConfig.contains("colorcycleduration")) {
					auto const colorcycleduration = backgroundConfig.at("colorcycleduration").get<unsigned>();

					theme->colorcycleduration = colorcycleduration;
				}
			}
			if (screenConfig.contains("images")) {
				auto const images = screenConfig.at("images");

				for (auto const& imageConfig : images) {
					if (!imageConfig.contains("image"))
						continue;

					auto image = Theme::Image();

					try {
						auto const fullpath = findFile(imageConfig.at("image").get<std::string>());

						image.texture = std::make_unique<Texture>(fullpath);
					}
					catch (std::exception const& e) {
						std::clog << "caught exception while loading texture for image: " << e.what() << std::endl;
						continue;
					}
					if (imageConfig.contains("x"))
						image.x = imageConfig.at("x").get<float>();
					if (imageConfig.contains("y"))
						image.y = imageConfig.at("y").get<float>();
					if (imageConfig.contains("scale"))
						image.scale = imageConfig.at("scale").get<float>();

					theme->images.emplace_back(std::move(image));
				}
			}
		}
	}
	catch (std::exception const& e) {
		std::clog << "caught exception while loading theme config: " << e.what() << std::endl;
	}

	return theme;
}
