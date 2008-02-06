#include "../config.h"

#include <audio.hpp>
#include <sdl_helper.h>
#include <screen.h>
#include <screen_intro.h>
#include <screen_songs.h>
#include <screen_sing.h>
#include <screen_practice.h>
#include <screen_score.h>
#include <screen_configuration.h>
#include <video_driver.h>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <set>
#include <string>
#include <vector>

SDL_Surface * screenSDL;

static void checkEvents_SDL(CScreenManager& sm) {
	static bool esc = false;
	SDL_Event event;
	while(SDL_PollEvent(&event) == 1) {
		switch(event.type) {
		  case SDL_QUIT:
			sm.finished();
			break;
		  case SDL_KEYUP:
			if (event.key.keysym.sym == SDLK_ESCAPE) esc = false;
			break;
		  case SDL_KEYDOWN:
			int keypressed  = event.key.keysym.sym;
			SDLMod modifier = event.key.keysym.mod;
			// Workaround for key repeat on escape
			if (keypressed == SDLK_ESCAPE) {
				if (esc) return;
				esc = true;
			}
			if (keypressed == SDLK_RETURN && modifier & KMOD_ALT ) {
				SDL_WM_ToggleFullScreen(screenSDL);
				sm.setFullscreenStatus(!sm.getFullscreenStatus());
				continue; // Already handled here...
			}
			break;
		}
		sm.getCurrentScreen()->manageEvent(event);
	}
}

static void init_SDL(CScreenManager& sm, CVideoDriver& vd, unsigned int width, unsigned int height) {
	std::atexit(SDL_Quit);
	if( SDL_Init(SDL_INIT_VIDEO) ==  -1 ) throw std::runtime_error("SDL_Init failed");
	SDL_WM_SetCaption(PACKAGE" - "VERSION, "WM_DEFAULT");
	SDLSurf icon(sm.getThemePathFile("icon.png"));
	SDL_WM_SetIcon(icon, NULL);
	screenSDL = vd.init(width, height, sm.getFullscreenStatus());
	if (!screenSDL) throw std::runtime_error("Cannot initialize screen");
	SDL_ShowCursor(SDL_DISABLE);
	SDL_EnableUNICODE(SDL_ENABLE);
	SDL_EnableKeyRepeat(80, 80);
}

int main(int argc, char** argv) {
	bool fullscreen = false;
	unsigned int width, height;
	std::string theme;
	std::set<std::string> songdirs;
	std::string cdev;
	std::size_t crate;
	{
		std::vector<std::string> songdirstmp;
		namespace po = boost::program_options;
		po::options_description desc("Available options");
		desc.add_options()
		  ("help,h", "you are viewing it")
		  ("theme,t", po::value<std::string>(&theme)->default_value("lima"), "set theme (name or absolute path)")
		  ("songdir,s", po::value<std::vector<std::string> >(&songdirstmp)->composing(), "additional song folders to scan\n  -s none to disable built-in defaults")
		  ("fs,f", "enable full screen mode")
		  ("width,W", po::value<unsigned int>(&width)->default_value(800), "set horizontal resolution")
		  ("height,H", po::value<unsigned int>(&height)->default_value(600), "set vertical resolution")
		  ("cdev", po::value<std::string>(&cdev), "set capture device (disable autodetection)\n  --cdev dev[:settings]\n  --cdev help for list of devices")
		  ("crate", po::value<std::size_t>(&crate)->default_value(48000), "set capture frequency\n  44100 and 48000 Hz are optimal")
		  ("version,v", "display version number");
		po::variables_map vm;
		try {
			po::store(po::parse_command_line(argc, argv, desc), vm);
			po::notify(vm);
		} catch (std::exception& e) {
			std::cout << desc << std::endl;
			std::cout << "ERROR: " << e.what() << std::endl;
			return 1;
		}
		if (vm.count("help")) {
			std::cout << desc << std::endl;
			return 0;
		}
		if (cdev == "help") {
			da::record::devlist_t l = da::record::devices();
			std::cout << "Recording devices:" << std::endl;
			for (da::record::devlist_t::const_iterator it = l.begin(); it != l.end(); ++it) {
				std::cout << boost::format("  %1% %|10t|%2%\n") % it->name() % it->desc();
			}
			return 0;
		}
		if (vm.count("version")) {
			std::cout << PACKAGE << ' ' << VERSION << std::endl;
			return 0;
		}
		if (vm.count("fs")) fullscreen = true;
		// Copy songdirstmp into songdirs
		bool defaultdirs = true;
		for (std::vector<std::string>::const_iterator it = songdirstmp.begin(); it != songdirstmp.end(); ++it) {
			std::string str = *it;
			if (str == "none") { defaultdirs = false; continue; }
			if (*str.rbegin() != '/') str += '/';
			songdirs.insert(str);
		}
		if (defaultdirs) {
			songdirs.insert(DATA_DIR "songs/");
			{
				char const* home = getenv("HOME");
				if (home) songdirs.insert(std::string(home) + "/.ultrastar/songs/");
			}
			songdirs.insert("/usr/share/games/ultrastar/songs/");
			songdirs.insert("/usr/share/games/ultrastar-ng/songs/");
			songdirs.insert("/usr/share/ultrastar/songs/");
			songdirs.insert("/usr/share/ultrastar-ng/songs/");
			songdirs.insert("/usr/local/share/games/ultrastar/songs/");
			songdirs.insert("/usr/local/share/games/ultrastar-ng/songs/");
			songdirs.insert("/usr/local/share/ultrastar/songs/");
			songdirs.insert("/usr/local/share/ultrastar-ng/songs/");
		}
	}
	try {
		// Initialize everything
		CScreenManager sm(width, height, theme);
		sm.setFullscreenStatus(fullscreen);
		CVideoDriver vd;
		init_SDL(sm, vd, width, height);
		sm.setSDLScreen(screenSDL);
		sm.setAudio(new CAudio());
		sm.setVideoDriver(&vd);
		sm.addScreen(new CScreenIntro("Intro", width, height));
		sm.addScreen(new CScreenSongs("Songs", width, height, songdirs));
		Capture capture(cdev, crate);
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

