#include <audio.h>
#include <SDL/SDL.h>

CAudio::CAudio()
{
	if(Mix_OpenAudio(48000, AUDIO_S16, 2, 4096)) {
		fprintf(stderr,"Mix_OpenAudio Error\n");
		SDL_Quit();
		exit(EXIT_FAILURE);
	}
	music=NULL;
}

CAudio::~CAudio()
{
	Mix_HaltMusic();
	Mix_FreeMusic(music);
	for(unsigned int i = 0 ; i < sounds.size() ; i++ )
		Mix_FreeChunk(sounds[i]);
	Mix_CloseAudio();
}

void CAudio::loadMusic( char * filename )
{
	if( isPlaying() )
		Mix_FadeOutMusic(500);
	if( music != NULL ) {
		Mix_FreeMusic(music);
		music=NULL;
	}
	music = Mix_LoadMUS(filename);
}

void CAudio::playMusic( char * filename )
{
	if( Mix_PlayingMusic() )
		Mix_FadeOutMusic(500);
	if( music != NULL ) {
		Mix_FreeMusic(music);
		music=NULL;
	}
	music = Mix_LoadMUS(filename);
	//while( !isPlaying() )
	Mix_PlayMusic(music, 1);
}

bool CAudio::isPlaying( void )
{
	if(Mix_PlayingMusic()) {
		return true;
	} else {
		return false;
	}
}

void CAudio::stopMusic()
{
	Mix_HaltMusic();
}

int CAudio::loadSound( char * filename )
{
	sounds.push_back(Mix_LoadWAV(filename));
	return sounds.size()-1;
}

void CAudio::playSound( int channel , int id )
{
	Mix_PlayChannel(channel,sounds[id],0);
}
