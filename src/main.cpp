#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h> 
#include <GL/gl.h>

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
char buff[1024];

SDL_Event event;
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

void swapBuffers( void )
{
	SDL_GL_SwapBuffers();
	glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, width, height, 0, -1, 100);
	glMatrixMode(GL_MODELVIEW);
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
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
	SDL_videoFlags |= SDL_OPENGL ;
	SDL_videoFlags |= SDL_GL_DOUBLEBUFFER;
	if ( videoInf->hw_available )
		SDL_videoFlags |= SDL_HWSURFACE;
	else
		SDL_videoFlags |= SDL_SWSURFACE;
	if ( videoInf->blit_hw )
		SDL_videoFlags |= SDL_HWACCEL;

	SDL_SetVideoMode(width, height, videoInf->vfmt->BitsPerPixel, SDL_videoFlags );


	SDL_ShowCursor(SDL_DISABLE);
	SDL_EnableUNICODE(SDL_ENABLE);
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, SDL_ENABLE );
	SDL_EnableKeyRepeat(125, 125);

	glEnable(GL_POINT_SMOOTH);
	glShadeModel( GL_SMOOTH );
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
}

int main( int arc , char ** argv)
{
	init();

	screenManager = new CScreenManager( width, height );
	CScreen * screen;

	screenManager->setAudio( new CAudio() );
	screenManager->setRecord( new CRecord() );
	
	screen = new CScreenIntro("Intro");
	screenManager->addScreen(screen);
	screen = new CScreenSongs("Songs");
	screenManager->addScreen(screen);
	screen = new CScreenSing("Sing");
	screenManager->addScreen(screen);

	screenManager->activateScreen("Intro");


	

	//SDL_Color black = {0, 0, 0,0};
	//TTF_Font *font = TTF_OpenFont("fonts/arial.ttf", 65);

	shot = screenManager->getAudio()->loadSound("sounds/shot.wav");
	explose = screenManager->getAudio()->loadSound("sounds/explose.wav");
	change = screenManager->getAudio()->loadSound("sounds/change.wav");

	float freq;
	int note;

	while( !screenManager->isFinished() ) {

		// Check keyboard events
		checkEvents();
		screenManager->getRecord()->compute();
		freq = screenManager->getRecord()->getFreq();
		note = screenManager->getRecord()->getNoteId();

		screenManager->getCurrentScreen()->draw();

		/*
		SDL_Surface * noteSurf = TTF_RenderUTF8_Blended(font, record->getNoteStr(note) , black);
		CSdlTexture * noteTex = new CSdlTexture(noteSurf);
		noteTex->draw(300,450,100);
		SDL_FreeSurface(noteSurf);
		delete noteTex;
		*/
		/*
		if(freq != 0.0) {
			glPointSize(15.0);
			glBegin(GL_POINTS);
				glColor4f(0.6,0.0,0.0,1.0);
				glVertex2d(400, height-(int)record->getFreq());
				glColor4f(0.0,0.8,0.0,1.0);
				glVertex2d(400, height-(int)record->getNoteFreq(note));
			glEnd();
		}
		*/
		swapBuffers();
	}

	delete screenManager;

	//TTF_CloseFont(font);
	TTF_Quit();
	SDL_Quit();
}
