#include "../config.h"

#include <screen.h>
#include <screen_intro.h>
#include <screen_songs.h>
#include <screen_sing.h>
#include <screen_practice.h>
#include <screen_score.h>
#include <screen_configuration.h>
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
		case SDL_KEYDOWN:
			int keypressed  = event.key.keysym.sym;
			SDLMod modifier = event.key.keysym.mod;
			if( keypressed == SDLK_f && modifier&KMOD_ALT ) {
				SDL_WM_ToggleFullScreen(screenSDL);
				screenManager->setFullscreenStatus(!screenManager->getFullscreenStatus());
				break;
			}
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

	screenSDL = videoDriver->init( width, height, screenManager->getFullscreenStatus() );

	if( screenSDL == NULL ) {
		fprintf(stderr,"Cannot initialize screen\n");
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

	SDL_ShowCursor(SDL_DISABLE);
	SDL_EnableUNICODE(SDL_ENABLE);
	SDL_EnableKeyRepeat(125, 125);
}

void usage( char * progname )
{
	fprintf(stdout,"Usage: %s [OPTIONS] [SONG_DIRECTORY]\n", progname);
	fprintf(stdout,"Options:\n");
	fprintf(stdout,"\n");
	fprintf(stdout," -W, --width (default: 640)  set window width\n");
	fprintf(stdout," -H, --height (default: 480) set window height\n");
	fprintf(stdout," -t, --theme                 set theme (theme name or absolute path to the\n");
	fprintf(stdout,"                             theme)\n");
	fprintf(stdout," -c, --capture-device        set sound capture device (none to disable)\n");
	fprintf(stdout," -f, --fullscreen            enable fullscreen video output\n");
	fprintf(stdout," -d, --difficulty            set difficulty level\n");
	fprintf(stdout,"                             (0: easy, 1:medium, 2:hard (default))\n");
	fprintf(stdout," -h, --help                  display this text and exit\n");
	fprintf(stdout," -v, --version               display version number and exit\n");
	exit(EXIT_SUCCESS);
}

int main( int argc, char ** argv )
{
	char * songs_directory = NULL;
	char * theme_name      = NULL;
	char * capture_device  = (char*)"default";
	CScreen * screen       = NULL;
	int ch                 = 0;
	unsigned int difficulty= 2;
	bool fullscreen        = false;

	static struct option long_options[] =
		{
		{"width",required_argument,NULL,'W'},
		{"height",required_argument,NULL,'H'},
		{"theme",required_argument,NULL,'t'},
		{"help",no_argument,NULL,'h'},
		{"capture-device",required_argument,NULL,'c'},
		{"version",no_argument,NULL,'v'},
		{"difficulty",required_argument,NULL,'d'},
		{"fullscreen",no_argument,NULL,'f'},
		{0, 0, 0, 0}
	};

	while ((ch = getopt_long(argc, argv, "t:W:H:hc:fd:v", long_options, NULL)) != -1) {
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
				if(!strncmp("none",optarg,5))
					capture=false;
				else
					capture_device=optarg;
				break;
			case 'f':
				fullscreen=true;
				break;
			case 'd':
				difficulty = atoi(optarg);
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

	if( theme_name != NULL )
		screenManager = new CScreenManager( width, height , songs_directory , theme_name );
	else
		screenManager = new CScreenManager( width, height , songs_directory );
	
	screenManager->setFullscreenStatus(fullscreen);

	init();
	
	// FIXME: captureDevice and capture variables not used
	Capture captureObj(48000);

	screenManager->setSDLScreen(screenSDL);
	screenManager->setAudio( new CAudio() );
	screenManager->setVideoDriver( videoDriver );
	screenManager->setDifficulty( difficulty );
	
	screen = new CScreenIntro("Intro");
	screenManager->addScreen(screen);
	screen = new CScreenSongs("Songs");
	screenManager->addScreen(screen);
	screen = new CScreenSing("Sing", captureObj.fft());
	screenManager->addScreen(screen);
	screen = new CScreenPractice("Practice", captureObj.fft());
	screenManager->addScreen(screen);
	screen = new CScreenScore("Score");
	screenManager->addScreen(screen);
	screen = new CScreenConfiguration("Configuration");
	screenManager->addScreen(screen);

	screenManager->activateScreen("Intro");

	while( !screenManager->isFinished() ) {
		checkEvents();
		videoDriver->blank();
		screenManager->getCurrentScreen()->draw();
		videoDriver->swap();
		SDL_Delay(10);
	}

	delete videoDriver;
	delete screenManager;
	delete[] songs_directory;

	SDL_Quit();
	return EXIT_SUCCESS;
}
