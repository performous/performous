#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_thread.h>

#include <sys/ioctl.h>
#include <linux/soundcard.h>
#include <fcntl.h>
#include <fftw3.h>
#include <math.h>

#include <screen.h>
#include <screen_intro.h>
#include <screen_songs.h>
#include <screen_sing.h>

unsigned int width=800;
unsigned int height=600;

SDL_Event event;
SDL_Surface * screenSDL;
CScreenManager *screenManager;

int shot;
int explose;
int change;

void checkEvents( void )
{
	while(SDL_PollEvent( &event ) == 1) {
		screenManager->getCurrentScreen()->manageEvent(event);
		switch(event.type) {
			case SDL_QUIT:
				screenManager->finished();
				break;
		}
	}
}

void init( void )
{
	if( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) ==  -1 ) {
		fprintf(stderr,"SDL_Init Error\n");
		SDL_Quit();
		exit(EXIT_FAILURE);
	}
	TTF_Init();

	SDL_WM_SetCaption("UltraStar-ng - 0.0.5", "WM_DEFAULT");
	const SDL_VideoInfo * videoInf = SDL_GetVideoInfo();

	unsigned SDL_videoFlags  = 0;
	if ( videoInf->hw_available )
		SDL_videoFlags |= SDL_HWSURFACE;
	else
		SDL_videoFlags |= SDL_SWSURFACE;
	if ( videoInf->blit_hw )
		SDL_videoFlags |= SDL_HWACCEL;

	screenSDL = SDL_SetVideoMode(width, height, videoInf->vfmt->BitsPerPixel, SDL_videoFlags );


	SDL_ShowCursor(SDL_DISABLE);
	SDL_EnableUNICODE(SDL_ENABLE);
	SDL_EnableKeyRepeat(125, 125);
}

int thread_func(void *unused)
{
	while( !screenManager->isFinished() ) {
		screenManager->getRecord()->compute();
	}
	return 1;
}

int main( int arc , char ** argv)
{
	init();

	screenManager = new CScreenManager( width, height );
	CScreen * screen;

	screenManager->setSDLScreen(screenSDL);
	screenManager->setAudio( new CAudio() );
	screenManager->setRecord( new CRecord() );
	
	screen = new CScreenIntro("Intro");
	screenManager->addScreen(screen);
	screen = new CScreenSongs("Songs");
	screenManager->addScreen(screen);
	screen = new CScreenSing("Sing");
	screenManager->addScreen(screen);

	screenManager->activateScreen("Intro");

	shot = screenManager->getAudio()->loadSound("sounds/shot.wav");
	explose = screenManager->getAudio()->loadSound("sounds/explose.wav");
	change = screenManager->getAudio()->loadSound("sounds/change.wav");

	SDL_Thread *thread = SDL_CreateThread(thread_func, NULL);

	while( !screenManager->isFinished() ) {
		SDL_FillRect(screenSDL,NULL,0xffffff);
		checkEvents();
		screenManager->getCurrentScreen()->draw();
		SDL_Flip(screenSDL);
		SDL_Delay(50);
	}

	SDL_WaitThread(thread, NULL);

	delete screenManager;

	TTF_Quit();
	SDL_Quit();
}
