#ifndef __SCREENSING_H__
#define __SCREENSING_H__

#include "../config.h"

#include <screen.h>
#include <pitch_graph.h>
#include <theme.h>
#include <video.h>

class CScreenSing : public CScreen {
	public:
	CScreenSing( char * name );
	~CScreenSing();
	void manageEvent( SDL_Event event );
	void draw(void);
	private:
	SDL_Surface * videoSurf;
	// Keeps the pitch tracking graphics
	// in separate surface
	PitchGraph pitchGraph;
	std::vector <TNote *> sentence;
        bool play;
	bool finished;
	unsigned int start;
        unsigned int song_pos;
	char sentenceNextSentence[128];
	CVideo * video;
        CThemeSing *theme;
#ifdef USE_OPENGL
        unsigned int theme_texture;
        unsigned int pitchgraph_texture;
        unsigned int bg_texture;
#endif
};

#endif
