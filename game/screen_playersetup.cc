#include "screen_playersetup.hh"
#include "players.hh"
#include "game.hh"
#include "i18n.hh"
#include "controllers.hh"
#include "database.hh"

ScreenPlayerSetup::ScreenPlayerSetup(Game& game, Players& players, Database const& database)
: Screen("PlayerSetup"), m_game(game), m_players(players), m_database(database),
  m_background(findFile("intro_bg.svg")) {
	initializeControls();
}

void ScreenPlayerSetup::manageEvent(input::NavEvent const& event) {
	const auto nav = event.button;

	if (nav == input::NavButton::CANCEL)
		m_game.activateScreen("Intro");
	else switch(nav) {
		case input::NavButton::RIGHT:
			m_control.onKey(Control::Key::Right);
			break;
		case input::NavButton::LEFT:
			m_control.onKey(Control::Key::Left);
			break;
		case input::NavButton::UP:
			m_control.onKey(Control::Key::Up);
			break;
		case input::NavButton::DOWN:
			m_control.onKey(Control::Key::Down);
			break;
		default:;
	}
}

void ScreenPlayerSetup::manageEvent(SDL_Event event) {
	if (event.type != SDL_KEYDOWN)
		return;

	const auto shift = (event.key.keysym.mod & KMOD_SHIFT) != 0;

	switch(event.key.keysym.scancode) {
		case SDL_SCANCODE_ESCAPE:
			m_control.onKey(Control::Key::Escape);
			break;
		case SDL_SCANCODE_RETURN:
			m_control.onKey(Control::Key::Return);
			break;
		case SDL_SCANCODE_SPACE:
			m_control.onKey(Control::Key::Space);
			break;
		case SDL_SCANCODE_BACKSPACE:
			m_control.onKey(Control::Key::BackSpace);
			break;
		case SDL_SCANCODE_DELETE:
			m_control.onKey(Control::Key::Delete);
			break;
		case SDL_SCANCODE_TAB:
			m_control.onKey(shift ? Control::Key::BackTab : Control::Key::Tab);
			break;
		case SDL_SCANCODE_0:
			m_control.onKey(Control::Key::Number0);
			break;
		case SDL_SCANCODE_1:
			m_control.onKey(Control::Key::Number1);
			break;
		case SDL_SCANCODE_2:
			m_control.onKey(Control::Key::Number2);
			break;
		case SDL_SCANCODE_3:
			m_control.onKey(Control::Key::Number3);
			break;
		case SDL_SCANCODE_4:
			m_control.onKey(Control::Key::Number4);
			break;
		case SDL_SCANCODE_5:
			m_control.onKey(Control::Key::Number5);
			break;
		case SDL_SCANCODE_6:
			m_control.onKey(Control::Key::Number6);
			break;
		case SDL_SCANCODE_7:
			m_control.onKey(Control::Key::Number7);
			break;
		case SDL_SCANCODE_8:
			m_control.onKey(Control::Key::Number8);
			break;
		case SDL_SCANCODE_9:
			m_control.onKey(Control::Key::Number9);
			break;
		case SDL_SCANCODE_A:
			m_control.onKey(shift ? Control::Key::A : Control::Key::a);
			break;
		case SDL_SCANCODE_B:
			m_control.onKey(shift ? Control::Key::B : Control::Key::b);
			break;
		case SDL_SCANCODE_C:
			m_control.onKey(shift ? Control::Key::C : Control::Key::c);
			break;
		case SDL_SCANCODE_D:
			m_control.onKey(shift ? Control::Key::D : Control::Key::d);
			break;
		case SDL_SCANCODE_E:
			m_control.onKey(shift ? Control::Key::E : Control::Key::e);
			break;
		case SDL_SCANCODE_F:
			m_control.onKey(shift ? Control::Key::F : Control::Key::f);
			break;
		case SDL_SCANCODE_G:
			m_control.onKey(shift ? Control::Key::G : Control::Key::g);
			break;
		case SDL_SCANCODE_H:
			m_control.onKey(shift ? Control::Key::H : Control::Key::h);
			break;
		case SDL_SCANCODE_I:
			m_control.onKey(shift ? Control::Key::I : Control::Key::i);
			break;
		case SDL_SCANCODE_J:
			m_control.onKey(shift ? Control::Key::J : Control::Key::j);
			break;
		case SDL_SCANCODE_K:
			m_control.onKey(shift ? Control::Key::K : Control::Key::k);
			break;
		case SDL_SCANCODE_L:
			m_control.onKey(shift ? Control::Key::L : Control::Key::l);
			break;
		case SDL_SCANCODE_M:
			m_control.onKey(shift ? Control::Key::M : Control::Key::m);
			break;
		case SDL_SCANCODE_N:
			m_control.onKey(shift ? Control::Key::N : Control::Key::n);
			break;
		case SDL_SCANCODE_O:
			m_control.onKey(shift ? Control::Key::O : Control::Key::o);
			break;
		case SDL_SCANCODE_P:
			m_control.onKey(shift ? Control::Key::P : Control::Key::p);
			break;
		case SDL_SCANCODE_Q:
			m_control.onKey(shift ? Control::Key::Q : Control::Key::q);
			break;
		case SDL_SCANCODE_R:
			m_control.onKey(shift ? Control::Key::R : Control::Key::r);
			break;
		case SDL_SCANCODE_S:
			m_control.onKey(shift ? Control::Key::S : Control::Key::s);
			break;
		case SDL_SCANCODE_T:
			m_control.onKey(shift ? Control::Key::T : Control::Key::t);
			break;
		case SDL_SCANCODE_U:
			m_control.onKey(shift ? Control::Key::U : Control::Key::u);
			break;
		case SDL_SCANCODE_V:
			m_control.onKey(shift ? Control::Key::V : Control::Key::v);
			break;
		case SDL_SCANCODE_W:
			m_control.onKey(shift ? Control::Key::W : Control::Key::w);
			break;
		case SDL_SCANCODE_X:
			m_control.onKey(shift ? Control::Key::X : Control::Key::x);
			break;
		case SDL_SCANCODE_Y:
			m_control.onKey(shift ? Control::Key::Y : Control::Key::y);
			break;
		case SDL_SCANCODE_Z:
			m_control.onKey(shift ? Control::Key::Z : Control::Key::z);
			break;
		default:;
	}
}

void ScreenPlayerSetup::draw() {
	m_players.update(); // Poll for new players
	m_background.draw();

	auto items = std::vector<Item>();

	for(auto i = 0U; i < m_players.count(); ++i) {
		items.emplace_back(m_players[i].name);
		items.back().setUserData(&m_players[i]);
	}

	//for(auto i = 0U; i < 15; ++i)
	//	items.emplace_back("Player " + std::to_string(i));

	m_playerList.setItems(items);

	m_control.draw(m_gc);
}

void ScreenPlayerSetup::enter() {
}

void ScreenPlayerSetup::exit() {
}

void ScreenPlayerSetup::initializeControls() {
	const auto verticalSpace = 0.05;
	const auto verticalOffset = -0.15;
	const auto horizontalSpace = 0.225;
	const auto horizontalOffset = -0.45;
	const auto lineHeight = 0.025;
	const auto listHeight = 1 + verticalOffset - 0.45;
	auto y = verticalOffset;

	m_control.addControl(m_playerList);
	m_playerList.setGeometry(horizontalOffset, y, horizontalSpace, listHeight);
	m_playerList.onSelectionChanged([&](List& list, size_t index, size_t){ m_name.setText(list.getItems()[index].toString());});
		auto const player = *list.getItems()[index].getUserData<PlayerItem*>();
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

	m_control.addControl(m_nameLabel);
	m_nameLabel.setGeometry(horizontalOffset + horizontalSpace + 0.01, y, horizontalSpace, lineHeight);
	m_nameLabel.setText(_("Name:"));

	m_control.addControl(m_name);
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

	m_control.addControl(m_avatarLabel);
	m_avatarLabel.setGeometry(horizontalOffset + horizontalSpace + 0.01, y, horizontalSpace, lineHeight);
	m_avatarLabel.setText(_("Avatar:"));

	m_control.addControl(m_avatar);
	m_avatar.setGeometry(horizontalOffset + (horizontalSpace + 0.01) * 2, y, horizontalSpace, horizontalSpace);

	y += horizontalSpace + lineHeight * 0.5;

	m_control.addControl(m_bestScoreLabel);
	m_bestScoreLabel.setGeometry(horizontalOffset + horizontalSpace + 0.01, y, horizontalSpace, lineHeight);
	m_bestScoreLabel.setText(_("Best score:"));

	m_control.addControl(m_bestScore);
	m_bestScore.setGeometry(horizontalOffset + (horizontalSpace + 0.01) * 2, y, horizontalSpace, lineHeight);
	m_bestScore.setText(_("na"));

	y += lineHeight + lineHeight * 0.5;

	m_control.addControl(m_bestSongLabel);
	m_bestSongLabel.setGeometry(horizontalOffset + horizontalSpace + 0.01, y, horizontalSpace, lineHeight);
	m_bestSongLabel.setText(_("Best song:"));

	m_control.addControl(m_bestSong);
	m_bestSong.setGeometry(horizontalOffset + (horizontalSpace + 0.01) * 2, y, horizontalSpace, lineHeight);
	m_bestSong.setText(_("na"));

	m_control.focusNext();
}



