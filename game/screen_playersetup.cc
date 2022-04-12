#include "screen_playersetup.hh"
#include "players.hh"
#include "game.hh"
#include "i18n.hh"
#include "controllers.hh"

ScreenPlayerSetup::ScreenPlayerSetup(Game& game, Players& players)
: Screen("PlayerSetup"), m_game(game), m_players(players), m_background(findFile("intro_bg.svg")) {
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

	for(auto i = 0U; i < m_players.count(); ++i)
		items.emplace_back(m_players[i].name);

	for(auto i = 0U; i < 15; ++i)
		items.emplace_back("Player " + std::to_string(i));

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

	m_control.addControl(m_playerList);
	m_playerList.setGeometry(horizontalOffset, verticalOffset, horizontalSpace, listHeight);

	m_control.focusNext();
}



