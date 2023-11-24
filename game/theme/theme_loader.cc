#include "theme_loader.hh"
#include "json.hh"
#include "value/json_to_value_converter.hh"
#include "utils/math.hh"

namespace {
	ThemePtr createTheme(std::string const& screen) {
		if (screen == "Global")
			return std::make_shared<ThemeGlobal>();
		if (screen == "Intro")
			return std::make_shared<ThemeIntro>();
		if (screen == "Songs")
			return std::make_shared<ThemeSongs>();
		if (screen == "Sing")
			return std::make_shared<ThemeSing>();
		if (screen == "Practice")
			return std::make_shared<ThemePractice>();
		if (screen == "AudioDevices")
			return std::make_shared<ThemeAudioDevices>();
		if (screen == "Paths")
			return std::make_shared<ThemePaths>();
		if (screen == "Players")
			return std::make_shared<ThemePlayers>();
		if (screen == "Playlist")
			return std::make_shared<ThemePlaylistScreen>();

		throw std::logic_error("creation of theme for screen '" + screen + "' is not implemented!");
	}

	Value getValue(nlohmann::json const& config, JsonToValueConverter& converter) {
		return converter.convert(config);
	}

	void loadTexture(Texture& texture, std::string const& filename) {
		try {
			auto const fullpath = findFile(filename);

			texture.load(fullpath);
		}
		catch (std::exception const& e) {
			std::clog << "theme/error: " << "caught exception while loading texture for theme: " << e.what() << std::endl;
		}
	}

	void addConstants(std::map<std::string, Value>& values, ConstantValueProviderPtr provider) {
		values["screen.left"] = value::Constant("screen.left", provider);
		values["screen.right"] = value::Constant("screen.right", provider);
		values["screen.top"] = value::Constant("screen.top", provider);
		values["screen.botton"] = value::Constant("screen.bottom", provider);
		values["screen.center"] = value::Constant("screen.center", provider);
		values["screen.middle"] = value::Constant("screen.middle", provider);
		values["image.left"] = value::Constant("image.left", provider);
		values["image.right"] = value::Constant("image.right", provider);
		values["image.top"] = value::Constant("image.top", provider);
		values["image.botton"] = value::Constant("image.bottom", provider);
		values["image.center"] = value::Constant("image.center", provider);
		values["image.middle"] = value::Constant("image.middle", provider);
		values["image.width"] = value::Constant("image.width", provider);
		values["image.height"] = value::Constant("image.height", provider);
		values["image.width_half"] = value::Constant("image.width_half", provider);
		values["image.height_half"] = value::Constant("image.height_half", provider);
		values["image.width_origin"] = value::Constant("image.width_origin", provider);
		values["image.height_origin"] = value::Constant("image.height_origin", provider);
	}
}

ThemePtr ThemeLoader::load(std::string const& screenName)
{
	auto theme = createTheme(screenName);
	auto converter = JsonToValueConverter(theme->values);

	addConstants(theme->values, m_provider);

	try {
		auto const fullpath = findFile("theme.json");
		auto const config = readJSON(fullpath);

		if (config.contains("values")) {
			auto const valuesConfig = config.at("values");
			//auto values = std::vector<nlohmann::json>(valuesConfig.items().begin(), valuesConfig.items().end());
			auto valuesToParse = std::map<std::string, nlohmann::json>();

			for (auto const& valueConfig : valuesConfig.items()) {
				valuesToParse[valueConfig.key()] = valueConfig.value();
			}

			auto changed = true;
			auto message = std::string{};

			while (!valuesToParse.empty() && changed) {
				auto values = valuesToParse;

				changed = false;
				message.clear();

				for (auto const& item : values) {
					try {
						auto const id = item.first;
						auto const value = getValue(item.second, converter);

						theme->values[id] = value;

						valuesToParse.erase(id);
						changed = true;
					}
					catch (std::exception const& e) {
						message += std::string{ "\n" } + e.what();
					}
				}
			}

			if (!changed) {
				std::clog << "theme/error: cannot solve all values!" << message << std::endl;
			}
		}
		if (config.contains(screenName)) {
			auto const screenConfig = config.at(screenName);

			if (screenConfig.contains("background")) {
				auto const backgroundConfig = screenConfig.at("background");

				if (backgroundConfig.contains("image")) {
					if (backgroundConfig.at("image").is_string()) {
						auto const filename = backgroundConfig.at("image").get<std::string>();

						loadTexture(*theme->bg, filename);
					}
					else if (backgroundConfig.at("image").is_array()) {
						for (auto const& image : backgroundConfig.at("image"))
							theme->addBackgroundImage(image.get<std::string>());
					}
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
			if (screenConfig.contains("logo")) {
				auto const logoConfig = screenConfig.at("logo");

				if (logoConfig.contains("drawlogo")) {
					auto const drawlogo = logoConfig.at("drawlogo").get<bool>();

					theme->drawlogo = drawlogo;
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
						std::clog << "theme/error: " << "caught exception while loading texture for image: " << e.what() << std::endl;
						continue;
					}
					if (imageConfig.contains("id"))
						image.id = imageConfig.at("id").get<std::string>();
					if (imageConfig.contains("x"))
						image.x = imageConfig.at("x").get<float>();
					if (imageConfig.contains("y"))
						image.y = imageConfig.at("y").get<float>();
					if (imageConfig.contains("z"))
						image.z = imageConfig.at("z").get<float>();
					if (imageConfig.contains("scale")) {
						if (imageConfig.at("scale").is_array()) {
							image.scaleHorizontal = getValue(imageConfig.at("scale").at(0), converter);
							image.scaleVertical = getValue(imageConfig.at("scale").at(1), converter);
						}
						else {
							image.scaleHorizontal = image.scaleVertical = getValue(imageConfig.at("scale"), converter);
						}
					}
					if (imageConfig.contains("alpha"))
						image.alpha = getValue(imageConfig.at("alpha"), converter);
					if (imageConfig.contains("angle"))
						image.angle = getValue(imageConfig.at("angle"), converter);

					theme->images.emplace_back(std::move(image));
				}
			}
			if (screenConfig.contains("events")) {
				auto const events = screenConfig.at("events");

				for (auto const& eventConfig : events.items()) {
					auto const eventName = eventConfig.key();
					auto event = Theme::Event();

					for (auto const& imageConfig : eventConfig.value().at("images")) {
						if (!imageConfig.contains("id"))
							continue;

						auto image = Theme::ImageConfig();

						image.id = imageConfig.at("id").get<std::string>();

						if (imageConfig.contains("x"))
							image.x = getValue(imageConfig.at("x"), converter);
						if (imageConfig.contains("y"))
							image.y = getValue(imageConfig.at("y"), converter);
						if (imageConfig.contains("z"))
							image.z = getValue(imageConfig.at("z"), converter);
						if (imageConfig.contains("scale")) {
							if (imageConfig.at("scale").is_array()) {
								image.scaleHorizontal = getValue(imageConfig.at("scale").at(0), converter);
								image.scaleVertical = getValue(imageConfig.at("scale").at(1), converter);
							}
							else {
								image.scaleHorizontal = image.scaleVertical = getValue(imageConfig.at("scale"), converter);
							}
						}
						if (imageConfig.contains("alpha"))
							image.alpha = getValue(imageConfig.at("alpha"), converter);
						if (imageConfig.contains("angle"))
							image.angle = getValue(imageConfig.at("angle"), converter);

						event.images.emplace_back(image);
					}

					theme->events[eventName] = std::move(event);
				}
			}
		}
	}
	catch (std::exception const& e) {
		std::clog << "theme/error: " << "caught exception while loading theme config: " << e.what() << std::endl;
	}

	return theme;
}

ThemeLoader::ThemeLoader(ConstantValueProviderPtr provider)
	: m_provider(provider) {
}
