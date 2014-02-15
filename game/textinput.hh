#pragma once
#include <SDL2/SDL.h>
#include <string>

/// type text
struct TextInput {
	/// text to operate on
	std::string text;
	/// processes keypresses
	bool process(SDL_Keysym const& key) {
		unsigned int ucs = key.sym;  //SDL2.0 uses unicode all the time, so replace with keycode
									/* Scancodes are meant to be layout-independent. Think of this as "the user pressed the Q key as it would be on a US QWERTY keyboard" regardless of whether this is actually a European keyboard or a Dvorak keyboard or whatever. The scancode is always the same key position.
									Keycodes are meant to be layout-dependent. Think of this as "the user pressed the key that is labelled 'Q' on his specific keyboard." */
		if (ucs == SDLK_LEFT) return false;
		if (ucs == SDLK_RIGHT) return false;
		if (ucs == SDLK_UP) return false;
		if (ucs == SDLK_DOWN) return false;
		if (ucs == SDLK_BACKSPACE && !text.empty()) backspace();
		else if(ucs!= SDLK_LALT && ucs!= SDLK_LCTRL && ucs!= SDLK_LSHIFT && ucs!= SDLK_RALT && ucs!= SDLK_RCTRL && ucs!= SDLK_END && ucs!= SDLK_HOME &&
			ucs!= SDLK_RSHIFT && ucs !=SDLK_PAGEDOWN && ucs!= SDLK_PAGEUP && ucs!= SDLK_RETURN && ucs!= SDLK_RETURN2 && ucs!= SDLK_ESCAPE && ucs != SDLK_BACKSPACE)
		*this += ucs; //I know this is less accurate, but better readable. the old one required you to hold ctrl or shift otherwise textinput didnt work.
		else return false;
		return true;
	}
	/// appends unicode symbol
	TextInput& operator+=(unsigned int ucs) { //don't know if this is still relevant since SDL2 handles unicode
		if (ucs < 0x80) {
			text += ucs;
		} else if (ucs < 0x800) {
			text += 0xC0 | (ucs >> 6);
			text += 0x80 | (ucs & 0x3F);
		} else if (ucs < 0x10000) {
			text += 0xE0 | (ucs >> 12);
			text += 0x80 | ((ucs >> 6) & 0x3F);
			text += 0x80 | (ucs & 0x3F);
		} else {
			text += 0xF0 | (ucs >> 18);
			text += 0x80 | ((ucs >> 12) & 0x3F);
			text += 0x80 | ((ucs >> 6) & 0x3F);
			text += 0x80 | (ucs & 0x3F);
		}
		return *this;
	}
	/// deletes last char
	void backspace() {
		if (text.empty()) return;
		std::string::size_type pos = text.size() - 1;
		while ((text[pos] & 0xC0) == 0x80) --pos;
		text.erase(pos);
	}
};

