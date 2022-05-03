#include "formscreen.hh"

FormScreen::FormScreen(const std::string& name)
: Screen(name) {
}

void FormScreen::manageEvent(input::NavEvent const& event) {
	const auto nav = event.button;

	if (nav == input::NavButton::CANCEL)
		onCancel();
	else switch(nav) {
		case input::NavButton::RIGHT:
			m_control.onKey(Control::Key::Right);
			m_control.onKeyUp(Control::Key::Right);
			break;
		case input::NavButton::LEFT:
			m_control.onKey(Control::Key::Left);
			m_control.onKeyUp(Control::Key::Left);
			break;
		case input::NavButton::UP:
			m_control.onKey(Control::Key::Up);
			m_control.onKeyUp(Control::Key::Up);
			break;
		case input::NavButton::DOWN:
			m_control.onKey(Control::Key::Down);
			m_control.onKeyUp(Control::Key::Down);
			break;
		default:;
	}
}

void FormScreen::manageEvent(SDL_Event event) {
	const auto shift = (event.key.keysym.mod & KMOD_SHIFT) != 0;
	const auto keyDown = event.type == SDL_KEYDOWN;
	const auto keyUp = event.type == SDL_KEYUP;

	if (!keyDown && !keyUp)
		return;

	auto sendKey = keyDown ? [](Control& control, Control::Key key){ control.onKeyDown(key);} : [](Control& control, Control::Key key){ control.onKeyUp(key); control.onKey(key);};

	switch(event.key.keysym.scancode) {
		case SDL_SCANCODE_ESCAPE:
			sendKey(m_control, Control::Key::Escape);
			break;
		case SDL_SCANCODE_RETURN:
			sendKey(m_control, Control::Key::Return);
			break;
		case SDL_SCANCODE_SPACE:
			sendKey(m_control, Control::Key::Space);
			break;
		case SDL_SCANCODE_BACKSPACE:
			sendKey(m_control, Control::Key::BackSpace);
			break;
		case SDL_SCANCODE_DELETE:
			sendKey(m_control, Control::Key::Delete);
			break;
		case SDL_SCANCODE_TAB:
			sendKey(m_control, shift ? Control::Key::BackTab : Control::Key::Tab);
			break;
		case SDL_SCANCODE_0:
			sendKey(m_control, Control::Key::Number0);
			break;
		case SDL_SCANCODE_1:
			sendKey(m_control, Control::Key::Number1);
			break;
		case SDL_SCANCODE_2:
			sendKey(m_control, Control::Key::Number2);
			break;
		case SDL_SCANCODE_3:
			sendKey(m_control, Control::Key::Number3);
			break;
		case SDL_SCANCODE_4:
			sendKey(m_control, Control::Key::Number4);
			break;
		case SDL_SCANCODE_5:
			sendKey(m_control, Control::Key::Number5);
			break;
		case SDL_SCANCODE_6:
			sendKey(m_control, Control::Key::Number6);
			break;
		case SDL_SCANCODE_7:
			sendKey(m_control, Control::Key::Number7);
			break;
		case SDL_SCANCODE_8:
			sendKey(m_control, Control::Key::Number8);
			break;
		case SDL_SCANCODE_9:
			sendKey(m_control, Control::Key::Number9);
			break;
		case SDL_SCANCODE_A:
			sendKey(m_control, shift ? Control::Key::A : Control::Key::a);
			break;
		case SDL_SCANCODE_B:
			sendKey(m_control, shift ? Control::Key::B : Control::Key::b);
			break;
		case SDL_SCANCODE_C:
			sendKey(m_control, shift ? Control::Key::C : Control::Key::c);
			break;
		case SDL_SCANCODE_D:
			sendKey(m_control, shift ? Control::Key::D : Control::Key::d);
			break;
		case SDL_SCANCODE_E:
			sendKey(m_control, shift ? Control::Key::E : Control::Key::e);
			break;
		case SDL_SCANCODE_F:
			sendKey(m_control, shift ? Control::Key::F : Control::Key::f);
			break;
		case SDL_SCANCODE_G:
			sendKey(m_control, shift ? Control::Key::G : Control::Key::g);
			break;
		case SDL_SCANCODE_H:
			sendKey(m_control, shift ? Control::Key::H : Control::Key::h);
			break;
		case SDL_SCANCODE_I:
			sendKey(m_control, shift ? Control::Key::I : Control::Key::i);
			break;
		case SDL_SCANCODE_J:
			sendKey(m_control, shift ? Control::Key::J : Control::Key::j);
			break;
		case SDL_SCANCODE_K:
			sendKey(m_control, shift ? Control::Key::K : Control::Key::k);
			break;
		case SDL_SCANCODE_L:
			sendKey(m_control, shift ? Control::Key::L : Control::Key::l);
			break;
		case SDL_SCANCODE_M:
			sendKey(m_control, shift ? Control::Key::M : Control::Key::m);
			break;
		case SDL_SCANCODE_N:
			sendKey(m_control, shift ? Control::Key::N : Control::Key::n);
			break;
		case SDL_SCANCODE_O:
			sendKey(m_control, shift ? Control::Key::O : Control::Key::o);
			break;
		case SDL_SCANCODE_P:
			sendKey(m_control, shift ? Control::Key::P : Control::Key::p);
			break;
		case SDL_SCANCODE_Q:
			sendKey(m_control, shift ? Control::Key::Q : Control::Key::q);
			break;
		case SDL_SCANCODE_R:
			sendKey(m_control, shift ? Control::Key::R : Control::Key::r);
			break;
		case SDL_SCANCODE_S:
			sendKey(m_control, shift ? Control::Key::S : Control::Key::s);
			break;
		case SDL_SCANCODE_T:
			sendKey(m_control, shift ? Control::Key::T : Control::Key::t);
			break;
		case SDL_SCANCODE_U:
			sendKey(m_control, shift ? Control::Key::U : Control::Key::u);
			break;
		case SDL_SCANCODE_V:
			sendKey(m_control, shift ? Control::Key::V : Control::Key::v);
			break;
		case SDL_SCANCODE_W:
			sendKey(m_control, shift ? Control::Key::W : Control::Key::w);
			break;
		case SDL_SCANCODE_X:
			sendKey(m_control, shift ? Control::Key::X : Control::Key::x);
			break;
		case SDL_SCANCODE_Y:
			sendKey(m_control, shift ? Control::Key::Y : Control::Key::y);
			break;
		case SDL_SCANCODE_Z:
			sendKey(m_control, shift ? Control::Key::Z : Control::Key::z);
			break;
		default:;
	}
}

void FormScreen::draw() {
	m_control.draw(m_gc);
}

