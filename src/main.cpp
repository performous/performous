#include "../config.h"

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
	
	SDL_WM_SetCaption(PACKAGE" - "VERSION, "WM_DEFAULT");
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

int thread_func(void *)
{
	while( !screenManager->isFinished() ) {
		screenManager->getRecord()->compute();
	}
	return 1;
}

void usage( char * progname )
{
	fprintf(stdout,"Usage: %s [options] song_directory\n", progname);
	fprintf(stdout,"Options:\n");
	fprintf(stdout,"--------\n");
	fprintf(stdout,"-h, --help\n");
	fprintf(stdout,"Display this text and exit\n");
	fprintf(stdout,"-W, --width (Default: 640)\n");
	fprintf(stdout,"Set window width\n");
	fprintf(stdout,"-H, --height (Default: 480)\n");
	fprintf(stdout,"Set window height\n");
	fprintf(stdout,"-t, --theme\n");
	fprintf(stdout,"Set theme\n");
	fprintf(stdout,"-v, --version\n");
	fprintf(stdout,"Display version number and exit\n");
	exit(EXIT_SUCCESS);
}

int main( int argc, char ** argv )
{
	char * songs_directory;
	char ch;

	static struct option long_options[] =
		{
		{"width",required_argument,NULL,'W'},
		{"height",required_argument,NULL,'H'},
		{"theme",required_argument,NULL,'t'},
		{"help",no_argument,NULL,'h'},
		{"version",no_argument,NULL,'v'},
		{0, 0, 0, 0}
	};

	while ((ch = getopt_long(argc, argv, "t:W:H:hv", long_options, NULL)) != -1) {
		switch(ch) {
			case 't':
				fprintf(stdout,"Theme selection is not yet available\n");
				break;
			case 'W':
				width=atoi(optarg);
				break;
			case 'H':
				height=atoi(optarg);
				break;
			case 'h':
				usage(argv[0]);
				break;
			case 'v':
				fprintf(stdout,"%s %s\n",PACKAGE, VERSION);
				exit(EXIT_SUCCESS);
				break;
		}
	}

	if( optind == argc ) {
		usage(argv[0]);
	}

	// Add the trailing slash
	songs_directory = new char[strlen(argv[optind])+2];
	sprintf(songs_directory,"%s/",argv[optind]);

	init();

	screenManager = new CScreenManager( width, height , songs_directory );
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
	delete[] songs_directory;

	SDL_Quit();
	return EXIT_SUCCESS;
}
