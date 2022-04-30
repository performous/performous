#include "screen_playersetup.hh"
#include "players.hh"
#include "game.hh"
#include "i18n.hh"
#include "controllers.hh"
#include "database.hh"

ScreenPlayerSetup::ScreenPlayerSetup(Game& game, Players& players, Database const& database)
: FormScreen("PlayerSetup"), m_game(game), m_players(players), m_database(database),
  m_background(findFile("intro_bg.svg")) {
	initializeControls();
}

void ScreenPlayerSetup::onCancel() {
	m_game.activateScreen("Intro");
}

void ScreenPlayerSetup::draw() {
	m_background.draw();
	FormScreen::draw();
}

void ScreenPlayerSetup::enter() {
	m_players.update(); // Poll for new players

	auto items = std::vector<Item>();

	for(auto i = 0U; i < m_players.count(); ++i) {
		auto& player = m_players[i];

		items.emplace_back(player.getName());
		items.back().setChecked(player.isActive());
		items.back().setUserData(&player);
	}

	//for(auto i = 0U; i < 15; ++i)
	//	items.emplace_back("Player " + std::to_string(i));

	m_playerList.setItems(items);
}

void ScreenPlayerSetup::exit() {
	for(auto const& item : m_playerList.getItems()) {
		auto& player = *item.getUserData<PlayerItem*>();

		std::cout << "player " << player.getName() << ": " << (item.isChecked() ? "checked" : "not checked") << std::endl;
		player.setActive(item.isChecked());

		std::cout << "player " << player.getName() << ": " << (player.isActive() ? "active" : "not active") << std::endl;
	}
}

void ScreenPlayerSetup::initializeControls() {
	const auto verticalSpace = 0.05;
	const auto verticalOffset = -0.15;
	const auto horizontalSpace = 0.225;
	const auto horizontalOffset = -0.45;
	const auto lineHeight = 0.025;
	const auto listHeight = 1 + verticalOffset - 0.45;
	auto y = verticalOffset;

	getForm().addControl(m_playerList);
	m_playerList.displayCheckBox(true);
	m_playerList.setGeometry(horizontalOffset, y, horizontalSpace, listHeight);
	m_playerList.onSelectionChanged([&](List& list, size_t index, size_t){ m_name.setText(list.getItems()[index].toString());});
		auto const& player = *list.getItems()[index].getUserData<PlayerItem*>();
		auto const path = player.getAvatarPath();

		if(path.empty())
			m_avatar.setTexture(findFile("no_player_image.svg"));
		else
			m_avatar.setTexture(path);

		auto const& scores = m_database.getHighScores();
		auto const result = scores.queryHiscore(-1, player.id, -1, {});
		auto const best = std::max_element(result.begin(), result.end(), [](auto const& a, auto const& b){ return a.score < b.score;});

		if(best == result.end()) {
			m_bestScore.setText(_("na"));
			m_bestSong.setText(_("na"));
		}
		else {
			auto const& songs = m_database.getSongs();
			auto const song = songs.lookup(best->songid);

			m_bestScore.setText(std::to_string(best->score));
			m_bestSong.setText(song);
		}

	getForm().addControl(m_nameLabel);
	m_nameLabel.setGeometry(horizontalOffset + horizontalSpace + 0.01, y, horizontalSpace, lineHeight);
	m_nameLabel.setText(_("Name:"));

	getForm().addControl(m_name);
	m_name.setMaxLength(12);
	m_name.setGeometry(horizontalOffset + (horizontalSpace + 0.01) * 2, y, horizontalSpace, lineHeight);
	m_name.onTextChanged(
		[&](TextBox&, std::string const& text, std::string const&){
			std::cout << "changed to " << text << std::endl;
			if(m_playerList.getSelectedIndex() != List::None) {
				m_playerList.getSelected().getUserData<PlayerItem*>()->name = text;
				m_playerList.getSelected().setText(text);
			}
		});

	y += lineHeight + lineHeight * 0.5;

	getForm().addControl(m_avatarLabel);
	m_avatarLabel.setGeometry(horizontalOffset + horizontalSpace + 0.01, y, horizontalSpace, lineHeight);
	m_avatarLabel.setText(_("Avatar:"));

	getForm().addControl(m_avatar);
	m_avatar.setGeometry(horizontalOffset + (horizontalSpace + 0.01) * 2, y, horizontalSpace, horizontalSpace);

	y += horizontalSpace + lineHeight * 0.5;

	getForm().addControl(m_bestScoreLabel);
	m_bestScoreLabel.setGeometry(horizontalOffset + horizontalSpace + 0.01, y, horizontalSpace, lineHeight);
	m_bestScoreLabel.setText(_("Best score:"));

	getForm().addControl(m_bestScore);
	m_bestScore.setGeometry(horizontalOffset + (horizontalSpace + 0.01) * 2, y, horizontalSpace, lineHeight);
	m_bestScore.setText(_("na"));

	y += lineHeight + lineHeight * 0.5;

	getForm().addControl(m_bestSongLabel);
	m_bestSongLabel.setGeometry(horizontalOffset + horizontalSpace + 0.01, y, horizontalSpace, lineHeight);
	m_bestSongLabel.setText(_("Best song:"));

	getForm().addControl(m_bestSong);
	m_bestSong.setGeometry(horizontalOffset + (horizontalSpace + 0.01) * 2, y, horizontalSpace, lineHeight);
	m_bestSong.setText(_("na"));

	getForm().focusNext();
}



