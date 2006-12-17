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
        int samplerate;
        samplerate = getMP3info(filename);
        
        if( Mix_PlayingMusic() )
		Mix_FadeOutMusic(500);
	if( music != NULL ) {
		Mix_FreeMusic(music);
		music=NULL;
	}
        Mix_CloseAudio();

        if(Mix_OpenAudio(samplerate, AUDIO_S16, 2, 4096)) {
            fprintf(stderr,"Mix_OpenAudio Error\n");
            SDL_Quit();
            exit(EXIT_FAILURE);
        }
        music = Mix_LoadMUS(filename);
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

void CAudio::stopMusic( void )
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
int CAudio::getMP3info( char * filename ) 
{
        FILE *pfile;
        unsigned char buffer[4], verid = 1;
        unsigned int samplerate = 0, filesize, pos = 0;
        pfile = fopen(filename, "rb");
        if (pfile == NULL) {
            fprintf(stderr,"could not open %s\n", filename);
            SDL_Quit();
            exit(EXIT_FAILURE);
        }
        
        fseek(pfile , 0 , SEEK_END);
        filesize = ftell (pfile);
        rewind (pfile);
        while(!samplerate && (pos < filesize - 4)) {
            fread(buffer, 1 , 4, pfile);
            fseek(pfile, pos, SEEK_SET);
            if ((buffer[0] == 0xFF) && (buffer[1] & 0xE0) == 0xE0) { /* frane found */
                switch (buffer[1] & 0x18) { /* audio version id */
                    case 0:
                        verid = 4;  /* MPEG Version 2.5 (later extension of MPEG 2) */
                        break;
                    case 16:
                        verid = 2; /* MPEG Version 2 (ISO/IEC 13818-3) */
                        break;
                    case 24:
                        verid = 1; /* MPEG Version 1 (ISO/IEC 11172-3) */
                        break;
                }
                switch (buffer[2] & 0x0C) { /* samplerate */
                    case 0:
                        samplerate = 44100 / verid;
                        break;
                    case 4:
                        samplerate = 48000 / verid;
                        break;
                    case 8:
                        samplerate = 32000 / verid;
                        break;
                }
            }
            pos++;
        }
        fclose (pfile);
        return samplerate;
}
