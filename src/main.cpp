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
#include <xtime.h>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <cstdlib>
#include <fstream>
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
		  case SDL_VIDEORESIZE:
			sm.getVideoDriver()->resize(event.resize.w, event.resize.h);
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
#ifdef HAVE_LIBSDL_IMAGE
	SDLSurf icon(sm.getThemePathFile("icon.png"));
	SDL_WM_SetIcon(icon, NULL);
#endif
	screenSDL = vd.init(width, height, sm.getFullscreenStatus());
	if (!screenSDL) throw std::runtime_error("Cannot initialize screen");
	SDL_ShowCursor(SDL_DISABLE);
	SDL_EnableUNICODE(SDL_ENABLE);
	SDL_EnableKeyRepeat(80, 80);
}

int main(int argc, char** argv) {
	std::ios::sync_with_stdio(false);  // We do not use C stdio
	srand(time(NULL));  // Seed for std::random_shuffle (used by song selector)
	bool fullscreen = false;
	bool fps = false;
	unsigned int width, height;
	std::string songlist;
	std::string theme;
	std::set<std::string> songdirs;
	std::string cdev;
	std::string pdev;
	std::size_t crate;
	std::size_t prate;
	std::string homedir;
	{
		char const* home = getenv("HOME");
		if (home) homedir = std::string(home) + '/';
	}
	{
		std::vector<std::string> songdirstmp;
		namespace po = boost::program_options;
		po::options_description opt1("Generic options");
		opt1.add_options()
		  ("help,h", "you are viewing it")
		  ("version,v", "display version number");
		po::options_description opt2("Configuration options");
		opt2.add_options()
		  ("theme,t", po::value<std::string>(&theme)->default_value("lima"), "set theme (name or absolute path)")
		  ("fs,f", "enable full screen mode")
		  ("fps", "benchmark rendering speed\n  also disable 100 FPS limit")
		  ("songlist", po::value<std::string>(&songlist), "print list of songs to console\n  --songlist=[title|artist|path]")
		  ("width,W", po::value<unsigned int>(&width)->default_value(800), "set horizontal resolution")
		  ("height,H", po::value<unsigned int>(&height)->default_value(600), "set vertical resolution")
		  ("cdev", po::value<std::string>(&cdev), "set capture device (disable autodetection)\n  --cdev=dev[:settings]\n  --cdev=help for list of devices")
		  ("pdev", po::value<std::string>(&pdev), "set playback device (disable autodetection)\n  --pdev=dev[:settings]\n  --pdev=help for list of devices")
		  ("crate", po::value<std::size_t>(&crate)->default_value(48000), "set capture frequency\n  44100 and 48000 Hz are optimal")
		  ("prate", po::value<std::size_t>(&prate)->default_value(48000), "set playback frequency\n  44100 and 48000 Hz are optimal")
		  ("clean,c", "disable internal default song folders")
		  ("songdir,s", po::value<std::vector<std::string> >(&songdirstmp)->composing(), "additional song folders to scan\n  may be specified without -s or -songdir too");
		po::positional_options_description p;
		p.add("songdir", -1);
		po::options_description cmdline;
		cmdline.add(opt1).add(opt2);
		po::variables_map vm;
		try {
			po::store(po::command_line_parser(argc, argv).options(cmdline).positional(p).run(), vm);
			if (!homedir.empty()) {
				std::ifstream conf((homedir + ".ultrastar/ultrastarng.conf").c_str());
				po::store(po::parse_config_file(conf, opt2), vm);
			}
			{
				std::ifstream conf("/etc/ultrastarng.conf");
				po::store(po::parse_config_file(conf, opt2), vm);
			}
			po::notify(vm);
		} catch (std::exception& e) {
			std::cout << cmdline << std::endl;
			std::cout << "ERROR: " << e.what() << std::endl;
			return 1;
		}
		if (vm.count("help")) {
			std::cout << cmdline << std::endl;
			return 0;
		}
		if (pdev == "help") {
			da::playback::devlist_t l = da::playback::devices();
			std::cout << "Playback devices:" << std::endl;
			for (da::playback::devlist_t::const_iterator it = l.begin(); it != l.end(); ++it) {
				std::cout << boost::format("  %1% %|10t|%2%\n") % it->name() % it->desc();
			}
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
		if (vm.count("fps")) fps = true;
		// Copy songdirstmp into songdirs
		for (std::vector<std::string>::const_iterator it = songdirstmp.begin(); it != songdirstmp.end(); ++it) {
			std::string str = *it;
			if (*str.rbegin() != '/') str += '/';
			songdirs.insert(str);
		}
		// Insert default dirs
		if (!vm.count("clean")) {
			songdirs.insert(DATA_DIR "songs/");
			if (!homedir.empty()) songdirs.insert(homedir + ".ultrastar/songs/");
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
		CScreenManager sm(theme);
		sm.setFullscreenStatus(fullscreen);
		CVideoDriver vd;
		init_SDL(sm, vd, width, height);
		sm.setSDLScreen(screenSDL);
		sm.setAudio(new CAudio(pdev));
		sm.setVideoDriver(&vd);
		sm.addScreen(new CScreenIntro("Intro"));
		sm.addScreen(new CScreenSongs("Songs", songdirs));
		Capture capture(cdev, crate);
		sm.addScreen(new CScreenSing("Sing", capture.analyzer()));
		sm.addScreen(new CScreenPractice("Practice", capture.analyzer()));
		sm.addScreen(new CScreenScore("Score"));
		sm.addScreen(new CScreenConfiguration("Configuration"));
		sm.activateScreen("Intro");
		// Main loop
		if (!songlist.empty()) sm.getSongs()->dump(std::cout, songlist);
		boost::xtime time = now();
		int frames = 0;
		while (!sm.isFinished()) {
			checkEvents_SDL(sm);
			vd.blank();
			sm.getCurrentScreen()->draw();
			vd.swap();
			++frames;
			if (fps) {
				if (now() - time > 1.0) {
					std::cout << frames << " FPS" << std::endl;
					time += 1.0;
					frames = 0;
				}
			} else {
				boost::thread::sleep(time + 0.01); // Max 100 FPS
				time = now();
			}
		}
	} catch (std::exception& e) {
		std::cout << "FATAL ERROR: " << e.what() << std::endl;
	}
}

