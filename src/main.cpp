#include "../config.h"

#include <screen.h>
#include <screen_intro.h>
#include <screen_songs.h>
#include <screen_sing.h>
#include <video_driver.h>
unsigned int width=800;
unsigned int height=600;

SDL_Event event;
SDL_Surface * screenSDL;
CScreenManager *screenManager;
CVideoDriver *videoDriver;

bool capture = true;

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
	if( SDL_Init(SDL_INIT_VIDEO) ==  -1 ) {
		fprintf(stderr,"SDL_Init Error\n");
		SDL_Quit();
		exit(EXIT_FAILURE);
	}
	
	SDL_WM_SetCaption(PACKAGE" - "VERSION, "WM_DEFAULT");

	screenSDL = videoDriver->init( width, height );

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
	fprintf(stdout,"Usage: %s [options] [song_directory]\n", progname);
	fprintf(stdout,"Options:\n");
	fprintf(stdout,"--------\n");
	fprintf(stdout,"-h, --help\n");
	fprintf(stdout,"\tDisplay this text and exit\n");
	fprintf(stdout,"-W, --width (Default: 640)\n");
	fprintf(stdout,"\tSet window width\n");
	fprintf(stdout,"-H, --height (Default: 480)\n");
	fprintf(stdout,"\tSet window height\n");
	fprintf(stdout,"-t, --theme\n");
	fprintf(stdout,"\tSet theme (theme name or absolute path to the theme)\n");
	fprintf(stdout,"-c, --no-capture\n");
	fprintf(stdout,"\tDisable sound capture thread\n");
	fprintf(stdout,"-v, --version\n");
	fprintf(stdout,"\tDisplay version number and exit\n");
	exit(EXIT_SUCCESS);
}

int main( int argc, char ** argv )
{
	char * songs_directory = NULL;
	char * theme_name      = NULL;
	CScreen * screen       = NULL;
	char ch                = 0;
	SDL_Thread *thread     = NULL;

	static struct option long_options[] =
		{
		{"width",required_argument,NULL,'W'},
		{"height",required_argument,NULL,'H'},
		{"theme",required_argument,NULL,'t'},
		{"help",no_argument,NULL,'h'},
		{"no-capture",no_argument,NULL,'c'},
		{"version",no_argument,NULL,'v'},
		{0, 0, 0, 0}
	};

	while ((ch = getopt_long(argc, argv, "t:W:H:hcv", long_options, NULL)) != -1) {
		switch(ch) {
			case 't':
				theme_name = optarg;
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
			case 'c':
				capture=false;
				break;
			case 'v':
				fprintf(stdout,"%s %s\n",PACKAGE, VERSION);
				exit(EXIT_SUCCESS);
				break;
		}
	}

	if( optind == argc ) {
		// Using default songs directory
		const char default_songs_directory[] = DATA_DIR "/songs";
		fprintf(stdout,"Using %s as default songs directory\n",default_songs_directory);
		songs_directory = new char[strlen(default_songs_directory)+2];
		sprintf(songs_directory,"%s/",default_songs_directory); // safe sprintf
	} else {
		// Add the trailing slash
		songs_directory = new char[strlen(argv[optind])+2];
		sprintf(songs_directory,"%s/",argv[optind]); // safe sprintf
	}

	videoDriver = new CVideoDriver();

	init();

	if( theme_name != NULL )
		screenManager = new CScreenManager( width, height , songs_directory , theme_name );
	else
		screenManager = new CScreenManager( width, height , songs_directory );

	screenManager->setSDLScreen(screenSDL);
	screenManager->setAudio( new CAudio() );
	screenManager->setRecord( new CRecord() );
	screenManager->setVideoDriver( videoDriver );
	
	screen = new CScreenIntro("Intro");
	screenManager->addScreen(screen);
	screen = new CScreenSongs("Songs");
	screenManager->addScreen(screen);
	screen = new CScreenSing("Sing");
	screenManager->addScreen(screen);

	screenManager->activateScreen("Intro");

	if( capture )
		thread = SDL_CreateThread(thread_func, NULL);

	while( !screenManager->isFinished() ) {
		checkEvents();
		videoDriver->blank();
                screenManager->getCurrentScreen()->draw();
		videoDriver->swap();
                SDL_Delay(50);
        }

	if( capture )
		SDL_WaitThread(thread, NULL);

	delete videoDriver;
	delete screenManager;
	delete[] songs_directory;

	SDL_Quit();
	return EXIT_SUCCESS;
}
