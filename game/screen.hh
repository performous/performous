#pragma once

#include "controllers.hh"
#include <SDL_events.h>
#include <cstdint>
#include <string>
#include <memory>

class Audio;
class Game;

/// Abstract Class for screens
class Screen {
  public:
	/// counstructor
	Screen(Game &game, std::string const& name): m_game(game), m_name(name) {}
	virtual ~Screen() = default;
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

  private:
	Game &m_game;
	std::string m_name;
};
