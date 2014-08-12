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
		if (ucs == SDL_SCANCODE_LEFT) return false;
		if (ucs == SDL_SCANCODE_RIGHT) return false;
		if (ucs == SDL_SCANCODE_UP) return false;
		if (ucs == SDL_SCANCODE_DOWN) return false;
		if (ucs == SDL_SCANCODE_BACKSPACE && !text.empty()) backspace();
		else if(ucs!= SDL_SCANCODE_LALT && ucs!= SDL_SCANCODE_LCTRL && ucs!= SDL_SCANCODE_LSHIFT && ucs!= SDL_SCANCODE_RALT && ucs!= SDL_SCANCODE_RCTRL && ucs!= SDL_SCANCODE_END && ucs!= SDL_SCANCODE_HOME &&
			ucs!= SDL_SCANCODE_RSHIFT && ucs !=SDL_SCANCODE_PAGEDOWN && ucs!= SDL_SCANCODE_PAGEUP && ucs!= SDL_SCANCODE_RETURN && ucs!= SDL_SCANCODE_RETURN2 && ucs!= SDL_SCANCODE_ESCAPE && ucs != SDL_SCANCODE_BACKSPACE)
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
	TextInput& operator+=(char cr[]) {
		text += cr;
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

