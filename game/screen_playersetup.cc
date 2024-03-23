#include "screen_playersetup.hh"
#include "players.hh"
#include "game.hh"
#include "i18n.hh"
#include "controllers.hh"
#include "database.hh"
#include <ui/imagemodifier/dimmer.hh>

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
				return static_cast<int>(i);
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
			avatar.setTexture(avatars[static_cast<size_t>(id)]);
	}
	void setAvatar(Image& avatar, Image& previousAvatar, Image& nextAvatar, int id, std::vector<std::string> const& avatars) {
		setAvatar(avatar, id, avatars);
		setAvatar(previousAvatar, id - 1, avatars);
		setAvatar(nextAvatar, id + 1, avatars);
	}
}

ScreenPlayerSetup::ScreenPlayerSetup(Game& game, Players& players, Database const& database)
: FormScreen(game, "PlayerSetup"), m_game(game), m_players(players), m_database(database),  m_background(findFile("intro_bg.svg")), m_grid(2, 1) {
	loadAvatars(m_avatars);
	initializeControls();
}

void ScreenPlayerSetup::onCancel() {
	m_game.activateScreen("Intro");
}

void ScreenPlayerSetup::draw() {
	m_background.draw(m_game.getWindow());
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
	//const auto listHeight = 1.f + verticalOffset - 0.5f;
	const auto verticalSpace = lineHeight * 0.125f;
	auto y = verticalOffset;

	getForm().addControl(m_grid);

	m_grid.resizeColumns({0.33f, 0.67f});
	m_grid.setGeometry(horizontalOffset, verticalOffset, -horizontalOffset * 2.0f, -verticalOffset * 2.0f);

	m_grid.setControl(0, 0, &m_playerList);
	m_grid.setControl(1, 0, &m_panel);

	m_playerList.displayIcon(true);
	m_playerList.displayCheckBox(true);
	m_playerList.setTabIndex(0);
	m_playerList.onSelectionChanged([&](List& list, size_t index, size_t){
		auto const& player = *list.getItems()[index].getUserData<PlayerItem*>();

		m_name.setText(player.getName());

		auto const avatar = player.getAvatar();

		if(avatar.empty())
			m_avatar.setTexture("no_player_image.svg");
		else
			m_avatar.setTexture(avatar);

		const auto id = getId(m_avatars, player.getAvatar());

		setAvatar(m_avatar, m_avatarPrevious, m_avatarNext, id, m_avatars);

		auto const& scores = m_database.getHighScores();
		auto const result = scores.queryHiscore(player.id.value(), {}, {});
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
			m_bestSong.setText(song.value());

			auto const averageScore = std::accumulate(result.begin(), result.end(), 0U,
				[](size_t sum, HiscoreItem const& score) { return sum + score.score;}) / result.size();

			m_averageScore.setText(std::to_string(averageScore));
		}
	});

	y = 0.f;

	m_panel.addControl(m_nameLabel);
	m_nameLabel.setGeometry(0.01f, y, horizontalLabelSpace, lineHeight);
	m_nameLabel.setText(_("Name:"));

	m_panel.addControl(m_name);
	m_name.setMaxLength(16);
	m_name.setTabIndex(1);
	m_name.setGeometry(0.01f + horizontalLabelSpace + 0.01f, y, horizontalSpace, lineHeight);
	m_name.onTextChanged(
		[&](TextBox&, std::string const& text, std::string const&){
			std::cout << "changed to " << text << std::endl;
			if(m_playerList.getSelectedIndex() != List::None) {
				m_playerList.getSelected().getUserData<PlayerItem*>()->name = text;
				m_playerList.getSelected().setText(text);
			}
		});

	y += lineHeight + verticalSpace;

	m_panel.addControl(m_avatarLabel);
	m_avatarLabel.setGeometry(0.01f, y, horizontalLabelSpace, lineHeight);
	m_avatarLabel.setText(_("Avatar:"));

	auto const sideSize = 0.75f;
	auto const avatarWidth = horizontalSpace * 0.6f;
	auto const avatarHeight = avatarWidth * 1.5f;
	auto const avatarSideWidth = avatarWidth * sideSize;
	auto const avatarSideHeight = avatarHeight * sideSize;
	auto x = 0.01f + horizontalLabelSpace + 0.01f;
	auto y2 = y + (avatarHeight - avatarSideHeight) * 0.5f;
	m_panel.addControl(m_avatarPrevious);
	m_avatarPrevious.setGeometry(x, y2, avatarSideWidth, avatarSideHeight);
	m_avatarPrevious.setModifier(std::make_unique<Dimmer>(0.7f));
	x += avatarSideWidth + horizontalSpace * 0.05f;
	m_panel.addControl(m_avatar);
	m_avatar.setGeometry(x, y, avatarWidth, avatarHeight);
	m_avatar.canBeFocused(true);
	m_avatar.setTabIndex(2);
	m_avatar.onKeyUp(
		[&](Control&, Control::Key key){
			if(key == Control::Key::Left)
				shiftAvatarLeft();
			else if(key == Control::Key::Right)
				shiftAvatarRight();
		}
	);
	x += avatarWidth + horizontalSpace * 0.05f;
	m_panel.addControl(m_avatarNext);
	m_avatarNext.setGeometry(x, y2, avatarSideWidth, avatarSideHeight);
	m_avatarNext.setModifier(std::make_unique<Dimmer>(0.7f));

	y += avatarHeight + verticalSpace;

	m_panel.addControl(m_bestScoreLabel);
	m_bestScoreLabel.setGeometry(0.01f, y, horizontalLabelSpace, lineHeight);
	m_bestScoreLabel.setText(_("Best score:"));

	m_panel.addControl(m_bestScore);
	m_bestScore.setGeometry(0.01f + horizontalLabelSpace + 0.01f, y, horizontalSpace, lineHeight);
	m_bestScore.setText(_("na"));

	y += lineHeight + verticalSpace;

	m_panel.addControl(m_bestSongLabel);
	m_bestSongLabel.setGeometry(0.01f, y, horizontalLabelSpace, lineHeight);
	m_bestSongLabel.setText(_("Best song:"));

	m_panel.addControl(m_bestSong);
	m_bestSong.setGeometry(0.01f + horizontalLabelSpace + 0.01f, y, horizontalSpace, lineHeight);
	m_bestSong.setText(_("na"));

	y += lineHeight + verticalSpace;

	m_panel.addControl(m_averageScoreLabel);
	m_averageScoreLabel.setGeometry(0.01f, y, horizontalLabelSpace, lineHeight);
	m_averageScoreLabel.setText(_("Average score:"));

	//getForm().addControl(m_averageScore);
	m_panel.addControl(m_averageScore);
	m_averageScore.setGeometry(0.01f + horizontalLabelSpace + 0.01f, y, horizontalSpace, lineHeight);
	m_averageScore.setText(_("na"));

	y += (lineHeight + verticalSpace) * 4;

	m_panel.addControl(m_addPlayerButton);
	m_addPlayerButton.setGeometry(0.01f, y, horizontalLabelSpace, lineHeight);
	m_addPlayerButton.setText(_("Add player"));
	m_addPlayerButton.setTabIndex(3);
	m_addPlayerButton.onClicked([&](Button&){addPlayer();});

	m_panel.addControl(m_deletePlayerButton);
	m_deletePlayerButton.setGeometry(0.01f + horizontalLabelSpace + 0.01f, y, horizontalLabelSpace, lineHeight);
	m_deletePlayerButton.setText(_("Delete player"));
	m_deletePlayerButton.setTabIndex(4);
	m_deletePlayerButton.onClicked([&](Button&){deletePlayer();});

	getForm().initialize(m_game);

	m_grid.layout();
	getForm().focus(m_playerList);

	if (m_players.isEmpty())
		setAvatar(m_avatar, m_avatarPrevious, m_avatarNext, 0, m_avatars);
}

int ScreenPlayerSetup::getCurrentId() const {
	if (m_playerList.countItems() > 0) {
		auto& player = *m_playerList.getSelected().getUserData<PlayerItem*>();
		auto avatar = player.getAvatar();
		auto id = getId(m_avatars, avatar);

		return id;
	}

	return m_avatarId;
}

void ScreenPlayerSetup::shiftAvatarLeft() {
	auto id = getCurrentId();

	std::cout << "current: " << id << std::endl;

	if(id == -1)
		id = static_cast<int>(m_avatars.size()) - 1;
	else
		--id;

	std::cout << "new current: " << id << std::endl;

	setAvatar(m_avatar, m_avatarPrevious, m_avatarNext, id, m_avatars);

	if (m_playerList.countItems() > 0) {
		auto& player = *m_playerList.getSelected().getUserData<PlayerItem*>();

		if (id == -1) {
			player.setAvatar("");
			player.setAvatarPath(findFile("no_player_image.svg"));
		}
		else {
			player.setAvatar(m_avatars[static_cast<size_t>(id)]);
			player.setAvatarPath(findFile(m_avatars[static_cast<size_t>(id)]));
		}

		m_playerList.getSelected().setIcon(player.getAvatarPath().string());
	}

	m_avatarId = id;
}

void ScreenPlayerSetup::shiftAvatarRight() {
	auto id = getCurrentId();

	std::cout << "current: " << id << std::endl;

	if(id == int(m_avatars.size()) - 1)
		id = -1;
	else
		++id;

	std::cout << "new current: " << id << std::endl;

	setAvatar(m_avatar, m_avatarPrevious, m_avatarNext, id, m_avatars);

	if (m_playerList.countItems() > 0) {
		auto& player = *m_playerList.getSelected().getUserData<PlayerItem*>();

		if (id == -1) {
			player.setAvatar("");
			player.setAvatarPath(findFile("no_player_image.svg"));
		}
		else {
			player.setAvatar(m_avatars[static_cast<size_t>(id)]);
			player.setAvatarPath(findFile(m_avatars[static_cast<size_t>(id)]));
		}

		m_playerList.getSelected().setIcon(player.getAvatarPath().string());
	}

	m_avatarId = id;
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


