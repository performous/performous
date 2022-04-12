#pragma once

#include "screen.hh"
#include "texture.hh"
#include "ui/form.hh"
#include "ui/graphiccontext.hh"
#include "ui/list.hh"

class Players;
class Game;

class ScreenPlayerSetup: public Screen {
  public:
	ScreenPlayerSetup(Game& game, Players& players);
	~ScreenPlayerSetup() override = default;

	void manageEvent(input::NavEvent const& event) override;
	void manageEvent(SDL_Event) override;
	void draw() override;
	void enter() override;
	void exit() override;

  private:
	void initializeControls();

  private:
	Game& m_game;
	Players& m_players;
	Texture m_background;
	GraphicContext m_gc;
	Form m_control;
	List m_playerList;
};
