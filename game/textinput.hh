#pragma once
#include <SDL.h>
#include <string>

/// type text
struct TextInput {
	/// text to operate on
	std::string text;
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

