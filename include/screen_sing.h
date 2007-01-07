#ifndef __SCREENSING_H__
#define __SCREENSING_H__

#include <screen.h>
#include <smpeg/smpeg.h>
#include <pitch_graph.h>
#include <theme.h>
class CScreenSing : public CScreen {
	public:
	CScreenSing( char * name );
	~CScreenSing();
	void manageEvent( SDL_Event event );
	void draw(void);
	private:
	SDL_Surface * title;
	SDL_Surface * videoSurf;
	// Keeps the pitch tracking graphics
	// in separate surface
	PitchGraph pitchGraph;
	std::vector <TNote *> sentence;
	bool play;
	bool finished;
	unsigned int start;
	SMPEG *mpeg;
	SMPEG_Info info;
        CThemeSing *theme;
};

#endif
