#include "../config.h"

#include <getopt.h>
#include <screen.h>
#include <screen_intro.h>
#include <screen_songs.h>
#include <screen_sing.h>
#include <screen_practice.h>
#include <screen_score.h>
#include <screen_configuration.h>
#include <video_driver.h>
#include <boost/thread.hpp>
#include <cstdlib>

unsigned int width=800;
unsigned int height=600;

#ifndef DATA_DIR
#define DATA_DIR "/usr/local/share/ultrastar-ng"
#endif

SDL_Surface * screenSDL;

void checkEvents_SDL(CScreenManager& sm) {
	SDL_Event event;
	while(SDL_PollEvent(&event) == 1) {
		sm.getCurrentScreen()->manageEvent(event);
		switch(event.type) {
		  case SDL_QUIT:
			sm.finished();
			break;
		  case SDL_KEYDOWN:
			int keypressed  = event.key.keysym.sym;
			SDLMod modifier = event.key.keysym.mod;
			if( keypressed == SDLK_f && modifier & KMOD_ALT ) {
				SDL_WM_ToggleFullScreen(screenSDL);
				sm.setFullscreenStatus(!sm.getFullscreenStatus());
				break;
			}
		}
	}
}

static void init_SDL(CScreenManager& sm, CVideoDriver& vd) {
	std::atexit(SDL_Quit);
	if( SDL_Init(SDL_INIT_VIDEO) ==  -1 ) throw std::runtime_error("SDL_Init failed");
	SDL_WM_SetCaption(PACKAGE" - "VERSION, "WM_DEFAULT");
	screenSDL = vd.init(width, height, sm.getFullscreenStatus());
	if (!screenSDL) throw std::runtime_error("Cannot initialize screen");
	SDL_ShowCursor(SDL_DISABLE);
	SDL_EnableUNICODE(SDL_ENABLE);
	SDL_EnableKeyRepeat(125, 125);
}

void usage(char* progname ) {
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
	std::exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
	char * theme_name      = NULL;
	char const* capture_device = "";
	unsigned long capture_rate = 48000;
	int ch                 = 0;
	unsigned int difficulty= 2;
	bool fullscreen        = false;

	static struct option long_options[] = {
		{"width",required_argument,NULL,'W'},
		{"height",required_argument,NULL,'H'},
		{"theme",required_argument,NULL,'t'},
		{"help",no_argument,NULL,'h'},
		{"capture-device",required_argument,NULL,'c'},
		{"capture-rate",required_argument,NULL,'r'},
		{"version",no_argument,NULL,'v'},
		{"difficulty",required_argument,NULL,'d'},
		{"fullscreen",no_argument,NULL,'f'},
		{0, 0, 0, 0}
	};

	while ((ch = getopt_long(argc, argv, "t:W:H:hc:r:fd:v", long_options, NULL)) != -1) {
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
				capture_device=optarg;
				break;
			case 'r':
				capture_rate=atoi(optarg);
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

	std::string songdir = DATA_DIR "songs/";
	if (optind != argc) {
		songdir = argv[optind];
		if (*songdir.rbegin() != '/') songdir += '/';
	}
	std::cout << "Using song directory " << songdir << std::endl;
	// Initialize everything
	try {
		CScreenManager sm(width, height, songdir.c_str(), theme_name);
		sm.setFullscreenStatus(fullscreen);
		CVideoDriver vd;
		init_SDL(sm, vd);
		sm.setSDLScreen(screenSDL);
		sm.setAudio(new CAudio());
		sm.setVideoDriver(&vd);
		sm.setDifficulty(difficulty);
		Capture capture(capture_device, capture_rate);
		sm.addScreen(new CScreenIntro("Intro", width, height));
		sm.addScreen(new CScreenSongs("Songs", width, height));
		sm.addScreen(new CScreenSing("Sing", width, height, capture.analyzer()));
		sm.addScreen(new CScreenPractice("Practice", width, height, capture.analyzer()));
		sm.addScreen(new CScreenScore("Score", width, height));
		sm.addScreen(new CScreenConfiguration("Configuration", width, height));
		sm.activateScreen("Intro");
		// Main loop
		while (!sm.isFinished()) {
			checkEvents_SDL(sm);
			vd.blank();
			sm.getCurrentScreen()->draw();
			vd.swap();
			boost::thread::yield();
		}
	} catch (std::exception& e) {
		std::cout << "FATAL ERROR: " << e.what() << std::endl;
	}
}

