#pragma once

#include "controllers.hh"
#include "game.hh"
#include "theme/theme_loader.hh"

#include <cstdint>
#include <string>
#include <memory>

#include <SDL_events.h>

class Audio;
class Game;

/// Abstract Class for screens
class Screen {
  public:
	/// counstructor
	Screen(Game &game, std::string const& name): m_game(game), m_name(name) {}
	virtual ~Screen() {}
	/// Event handler for navigation events
	virtual void manageEvent(input::NavEvent const& event) = 0;
	/// Event handler for SDL events
	virtual void manageEvent(SDL_Event) {}
	/// prepare screen for drawing
	virtual void prepare() {}
	/// draws screen
	virtual void draw() = 0;
	/// enters screen
	virtual void enter() = 0;
	/// exits screen
	virtual void exit() = 0;
	/// reloads OpenGL textures but avoids restarting music etc.
	virtual void reloadGL() { exit(); enter(); }
	/// returns screen name
	std::string getName() const { return m_name; }
	/// returns game
	Game& getGame() const { return m_game; }

	void drawImages(Theme const&);
	void drawImages(std::vector<Theme::Image> const&);

	void setTheme(std::shared_ptr<Theme>);
	std::shared_ptr<Theme> getTheme();
  protected:

	void setBackground(std::shared_ptr<Texture>);
	void drawBackground();

protected:
	Theme::Image* findImage(std::string const& id, Theme& theme);
	template<class ThemeType> std::shared_ptr<ThemeType> load()
	{
		auto loader = ThemeLoader(m_game.getConstantValueProvider());
		auto theme = loader.load<ThemeType>(getName());

		if (!theme)
			theme = std::make_unique<ThemeType>();

		m_theme = theme;

		return theme;
	}

  private:
	Game &m_game;
	std::string m_name;
	std::shared_ptr<Theme> m_theme;
	std::shared_ptr<Texture> m_background;
};
