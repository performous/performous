#ifndef __AUDIO_H_
#define __AUDIO_H_

#include "SDL/SDL_mixer.h"
#include <vector>

class CAudio {
	public:
	CAudio();
	~CAudio();
	void loadMusic( char * filename) ;
	void playMusic( char * filename );
	bool isPlaying( void );
	void stopMusic( void );
	int loadSound( char * filename );
	void playSound( int channel , int id );
	private:
	Mix_Music *music;
	std::vector <Mix_Chunk *> sounds;
};

#endif
