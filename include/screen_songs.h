#ifndef __SCREENSONGS_H__
#define __SCREENSONGS_H__

#include <screen.h>
#include <songs.h>
#include <theme.h>

class CScreenSongs : public CScreen {
  public:
	CScreenSongs(char const* name, unsigned int width, unsigned int height, std::set<std::string> const& songdirs);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();
  private:
	CThemeSongs* theme;
	int songId;
	bool play;
	bool searchMode;
	char* searchExpr;
	unsigned int bg_texture;
};

#endif

