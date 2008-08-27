#include <SDL.h>
#include <string>

struct TextInput {
	std::string text;
	bool process(SDL_keysym const& key) {
		unsigned int ucs = key.unicode;
		if (key.sym == SDLK_LEFT) return false;
		if (key.sym == SDLK_RIGHT) return false;
		if (key.sym == SDLK_UP) return false;
		if (key.sym == SDLK_DOWN) return false;
		if (key.sym == SDLK_BACKSPACE && !text.empty()) backspace();
		else if ((ucs == 0x20 && !text.empty()) || (ucs > 0x20 && (ucs < 0x7F || ucs >= 0xA0))) *this += ucs;
		else return false;
		return true;
	}
	TextInput& operator+=(unsigned int ucs) {
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
	void backspace() {
		if (text.empty()) return;
		std::string::size_type pos = text.size() - 1;
		while ((text[pos] & 0xC0) == 0x80) --pos;
		text.erase(pos);
	}
};

