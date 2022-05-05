#pragma once

#include "screen.hh"
#include "texture.hh"
#include "ui/form.hh"
#include "ui/graphiccontext.hh"
#include "ui/list.hh"
#include "ui/textbox.hh"
#include "ui/image.hh"
#include "ui/label.hh"

class Players;
class Game;
class Database;

class ScreenPlayerSetup: public Screen {
  public:
	ScreenPlayerSetup(Game& game, Players& players, Database const&);
	~ScreenPlayerSetup() override = default;

	void manageEvent(input::NavEvent const& event) override;
	void manageEvent(SDL_Event) override;
	void draw() override;
	void enter() override;
	void exit() override;

  private:
	void initializeControls();
	void shiftAvatarLeft();
	void shiftAvatarRight();

  private:
	Game& m_game;
	Players& m_players;
	Database const& m_database;
	Texture m_background;
	GraphicContext m_gc;
	Form m_control;
	List m_playerList;
	Label m_nameLabel;
	TextBox m_name;
	Label m_avatarLabel;
	Image m_avatar;
	Image m_avatarPrevious;
	Image m_avatarNext;
	Label m_bestScoreLabel;
	Label m_bestScore;
	Label m_bestSongLabel;
	Label m_bestSong;
	Label m_averageScoreLabel;
	Label m_averageScore;
	std::vector<std::string> m_avatars;
};
