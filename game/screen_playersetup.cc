#include "screen_playersetup.hh"
#include "players.hh"
#include "game.hh"
#include "i18n.hh"
#include "controllers.hh"
#include "database.hh"

namespace {
	void loadAvatars(std::vector<std::string>& avatars) {
		for(auto const& path : listFiles("avatar")) {
			std::cout << path << " : " << ("avatar" / path.filename()) << std::endl;
			avatars.emplace_back(("avatar" / path.filename()).string());
		}
	}
	int getId(std::vector<std::string> const& avatars, std::string const& current) {
		if(current.empty())
			return -1;

		for(auto i = 0U; i < avatars.size(); ++i) {
			std::cout << avatars[i] << " = " << current << std::endl;
			if(avatars[i] == current)
				return i;
		}

		return -1;
	}
	void setAvatar(Image& avatar, int id, std::vector<std::string> const& avatars) {
		if(id == -1)
			avatar.setTexture("no_player_image.svg");
		else if(id == -2)
			setAvatar(avatar, int(avatars.size()) - 1, avatars);
		else if(id == int(avatars.size()))
			setAvatar(avatar, -1, avatars);
		else
			avatar.setTexture(avatars[id]);
	}
	void setAvatar(Image& avatar, Image& previousAvatar, Image& nextAvatar, int id, std::vector<std::string> const& avatars) {
		setAvatar(avatar, id, avatars);
		setAvatar(previousAvatar, id - 1, avatars);
		setAvatar(nextAvatar, id + 1, avatars);
	}
}

ScreenPlayerSetup::ScreenPlayerSetup(Game& game, Players& players, Database const& database)
: FormScreen("PlayerSetup"), m_game(game), m_players(players), m_database(database),  m_background(findFile("intro_bg.svg")) {
	loadAvatars(m_avatars);
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
	m_players.update();

	initialize();

	getForm().focus(m_playerList);
}

void ScreenPlayerSetup::initialize() {
	auto items = std::vector<Item>();

	for(auto& p : m_players.getPlayers()) {
		auto& player = *p;
		std::cout << "add player " << player.getName() << std::endl;

		items.emplace_back(player.getName());
		items.back().setChecked(player.isActive());
		items.back().setUserData(&player);
		auto const avatar = player.getAvatar();
		if(avatar.empty())
			items.back().setIcon("no_player_image.svg");
		else
			items.back().setIcon(player.getAvatarPath().string());

		std::cout << player.getAvatar() << " : " << player.getAvatarPath() << std::endl;
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
	const auto verticalOffset = -0.15f;
	const auto horizontalSpace = 0.225f;
	const auto horizontalLabelSpace = horizontalSpace * 0.75f;
	const auto horizontalOffset = -0.45f;
	const auto lineHeight = 0.025f;
	const auto listHeight = 1.f + verticalOffset - 0.5f;
	const auto verticalSpace = lineHeight * 0.125f;
	auto y = verticalOffset;

	getForm().addControl(m_playerList);
	m_playerList.displayIcon(true);
	m_playerList.displayCheckBox(true);
	m_playerList.setGeometry(horizontalOffset, y, horizontalSpace, listHeight);
	m_playerList.onSelectionChanged([&](List& list, size_t index, size_t){ m_name.setText(list.getItems()[index].toString());});
		auto const& player = *list.getItems()[index].getUserData<PlayerItem*>();
		auto const avatar = player.getAvatar();

		if(avatar.empty())
			m_avatar.setTexture("no_player_image.svg");
		else
			m_avatar.setTexture(avatar);

		const auto id = getId(m_avatars, player.getAvatar());

		setAvatar(m_avatar, m_avatarPrevious, m_avatarNext, id, m_avatars);

		auto const& scores = m_database.getHighScores();
		auto const result = scores.queryHiscore(-1, player.id, -1, {});
		auto const best = std::max_element(result.begin(), result.end(), [](auto const& a, auto const& b){ return a.score < b.score;});

		if(best == result.end()) {
			m_bestScore.setText(_("na"));
			m_bestSong.setText(_("na"));
			m_averageScore.setText(_("na"));
		}
		else {
			auto const& songs = m_database.getSongs();
			auto const song = songs.lookup(best->songid);

			m_bestScore.setText(std::to_string(best->score));
			m_bestSong.setText(song);

			auto const averageScore = std::accumulate(result.begin(), result.end(), 0U,
				[](size_t sum, HiscoreItem const& score) { return sum + score.score;}) / result.size();

			m_averageScore.setText(std::to_string(averageScore));
		}

	getForm().addControl(m_nameLabel);
	m_nameLabel.setGeometry(horizontalOffset + horizontalSpace + 0.01f, y, horizontalLabelSpace, lineHeight);
	m_nameLabel.setText(_("Name:"));

	getForm().addControl(m_name);
	m_name.setMaxLength(16);
	m_name.setGeometry(horizontalOffset + horizontalSpace + 0.01f + horizontalLabelSpace + 0.01f, y, horizontalSpace, lineHeight);
	m_name.onTextChanged(
		[&](TextBox&, std::string const& text, std::string const&){
			std::cout << "changed to " << text << std::endl;
			if(m_playerList.getSelectedIndex() != List::None) {
				m_playerList.getSelected().getUserData<PlayerItem*>()->name = text;
				m_playerList.getSelected().setText(text);
			}
		});

	y += lineHeight + verticalSpace;

	getForm().addControl(m_avatarLabel);
	m_avatarLabel.setGeometry(horizontalOffset + horizontalSpace + 0.01f, y, horizontalLabelSpace, lineHeight);
	m_avatarLabel.setText(_("Avatar:"));

	auto x = horizontalOffset + horizontalSpace + 0.01f + horizontalLabelSpace + 0.01f;
	auto y2 = y + (horizontalSpace * (0.6f - 0.15f)) * 0.5f;
	getForm().addControl(m_avatarPrevious);
	m_avatarPrevious.setGeometry(x, y2, horizontalSpace * 0.15f, horizontalSpace * 0.15f);
	x += horizontalSpace * 0.2f;
	getForm().addControl(m_avatar);
	m_avatar.setGeometry(x, y, horizontalSpace * 0.6f, horizontalSpace * 0.6f);
	m_avatar.canBeFocused(true);
	m_avatar.onKeyUp(
		[&](Control&, Control::Key key){
			if(key == Control::Key::Left)
				shiftAvatarLeft();
			else if(key == Control::Key::Right)
				shiftAvatarRight();
		}
	);
	x += horizontalSpace * 0.65f;
	getForm().addControl(m_avatarNext);
	m_avatarNext.setGeometry(x, y2, horizontalSpace * 0.15f, horizontalSpace * 0.15f);

	y += horizontalSpace * 0.6f + verticalSpace;

	getForm().addControl(m_bestScoreLabel);
	m_bestScoreLabel.setGeometry(horizontalOffset + horizontalSpace + 0.01f, y, horizontalLabelSpace, lineHeight);
	m_bestScoreLabel.setText(_("Best score:"));

	getForm().addControl(m_bestScore);
	m_bestScore.setGeometry(horizontalOffset + horizontalSpace + 0.01f + horizontalLabelSpace + 0.01f, y, horizontalSpace, lineHeight);
	m_bestScore.setText(_("na"));

	y += lineHeight + verticalSpace;

	getForm().addControl(m_bestSongLabel);
	m_bestSongLabel.setGeometry(horizontalOffset + horizontalSpace + 0.01f, y, horizontalLabelSpace, lineHeight);
	m_bestSongLabel.setText(_("Best song:"));

	getForm().addControl(m_bestSong);
	m_bestSong.setGeometry(horizontalOffset + horizontalSpace + 0.01f + horizontalLabelSpace + 0.01f, y, horizontalSpace, lineHeight);
	m_bestSong.setText(_("na"));

	y += lineHeight + verticalSpace;

	getForm().addControl(m_averageScoreLabel);
	m_averageScoreLabel.setGeometry(horizontalOffset + horizontalSpace + 0.01f, y, horizontalLabelSpace, lineHeight);
	m_averageScoreLabel.setText(_("Average score:"));

	getForm().addControl(m_averageScore);
	m_averageScore.setGeometry(horizontalOffset + horizontalSpace + 0.01f + horizontalLabelSpace + 0.01f, y, horizontalSpace, lineHeight);
	m_averageScore.setText(_("na"));

	y += (lineHeight + verticalSpace) * 4;

	getForm().addControl(m_addPlayerButton);
	m_addPlayerButton.setGeometry(horizontalOffset + horizontalSpace + 0.01f, y, horizontalLabelSpace, lineHeight);
	m_addPlayerButton.setText(_("Add player"));
	m_addPlayerButton.onClicked([&](Button&){addPlayer();});

	getForm().addControl(m_deletePlayerButton);
	m_deletePlayerButton.setGeometry(horizontalOffset + horizontalSpace + 0.01f + horizontalLabelSpace + 0.01f, y, horizontalLabelSpace, lineHeight);
	m_deletePlayerButton.setText(_("Delete player"));
	m_deletePlayerButton.onClicked([&](Button&){deletePlayer();});

	getForm().focus(m_playerList);
}

void ScreenPlayerSetup::shiftAvatarLeft() {
	auto& player = *m_playerList.getSelected().getUserData<PlayerItem*>();
	auto avatar = player.getAvatar();
	auto id = getId(m_avatars, avatar);

	std::cout << "current: " << id << std::endl;

	if(id == -1)
		id = m_avatars.size() - 1;
	else
		--id;

	std::cout << "new current: " << id << std::endl;

	if(id == -1) {
		player.setAvatar("");
		player.setAvatarPath(findFile("no_player_image.svg"));
	}
	else {
		player.setAvatar(m_avatars[id]);
		player.setAvatarPath(findFile(m_avatars[id]));
	}

	setAvatar(m_avatar, m_avatarPrevious, m_avatarNext, id, m_avatars);
	m_playerList.getSelected().setIcon(player.getAvatarPath().string());
}

void ScreenPlayerSetup::shiftAvatarRight() {
	auto& player = *m_playerList.getSelected().getUserData<PlayerItem*>();
	auto avatar = player.getAvatar();
	auto id = getId(m_avatars, avatar);

	std::cout << "current: " << id << std::endl;

	if(id == int(m_avatars.size()) - 1)
		id = -1;
	else
		++id;

	std::cout << "new current: " << id << std::endl;

	if(id == -1) {
		player.setAvatar("");
		player.setAvatarPath(findFile("no_player_image.svg"));
	}
	else {
		player.setAvatar(m_avatars[id]);
		player.setAvatarPath(findFile(m_avatars[id]));
	}

	setAvatar(m_avatar, m_avatarPrevious, m_avatarNext, id, m_avatars);
	m_playerList.getSelected().setIcon(player.getAvatarPath().string());
}

void ScreenPlayerSetup::addPlayer() {
	auto const id = m_players.addPlayer(_("New player"));

	std::cout << "added player with id " << id << std::endl;

	initialize();

	auto const selector = [id](Item const& item) {
		return item.getUserData<PlayerItem*>()->id == id;
	};

	m_playerList.select(selector);

	getForm().focus(m_name);
}

void ScreenPlayerSetup::deletePlayer() {
	std::cout << "delete player" << std::endl;
	try {
		auto const& listitemToRemove = m_playerList.getSelected();
		auto const& playerToRemove = *listitemToRemove.getUserData<PlayerItem*>();

		m_players.removePlayer(playerToRemove);

		initialize();
	}
	catch(...) {}
}

