#include "theme_loader.hh"
#include "json.hh"

namespace {
	ThemePtr createTheme(std::string const& screen) {
		if (screen == "Intro")
			return std::make_shared<ThemeIntro>();

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

ThemePtr ThemeLoader::load(std::string const& screen)
{
	auto theme = createTheme(screen);

	try {
		auto const fullpath = findFile("theme.json");
		auto const config = readJSON(fullpath);

		if (config.contains(screen)) {
			auto introScreen = std::static_pointer_cast<ThemeIntro>(theme);
			auto const screenConfig = config.at(screen);

			if (screenConfig.contains("background")) {
				auto const filename = screenConfig.at("background").get<std::string>();

				loadTexture(introScreen->bg, filename);
			}
			if (screenConfig.contains("colorcycling")) {
				auto const colorcycling = screenConfig.at("colorcycling").get<bool>();

				introScreen->colorcycling = colorcycling;
			}
			if (screenConfig.contains("colorcycleduration")) {
				auto const colorcycleduration = screenConfig.at("colorcycleduration").get<unsigned>();

				introScreen->colorcycleduration = colorcycleduration;
			}
		}
	}
	catch (std::exception const& e) {
		std::clog << "caught exception while loading theme config: " << e.what() << std::endl;
	}

	return theme;
}
