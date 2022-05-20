#pragma once

#include "screen.hh"
#include "texture.hh"
#include "ui/button.hh"
#include "ui/formscreen.hh"
#include "ui/image.hh"
#include "ui/label.hh"
#include "ui/list.hh"
#include "ui/textbox.hh"

class Players;
class Game;
class Database;

class ScreenPlayerSetup: public FormScreen {
  public:
	ScreenPlayerSetup(Game& game, Players& players, Database const&);
	~ScreenPlayerSetup() override = default;

	void draw() override;
	void enter() override;
	void exit() override;

  protected:
	void onCancel() override;

  private:
	void initializeControls();
	void initialize();
	void shiftAvatarLeft();
	void shiftAvatarRight();
	void addPlayer();
	void deletePlayer();

  private:
	Game& m_game;
	Players& m_players;
	Database const& m_database;
	Texture m_background;
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
	Button m_addPlayerButton;
	Button m_deletePlayerButton;
	std::vector<std::string> m_avatars;
};
