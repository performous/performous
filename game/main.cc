#include "config.hh"
#include "fs.hh"
#include "screen.hh"
#include "joystick.hh"
#include "songs.hh"
#include "backgrounds.hh"
#include "database.hh"
#include "xtime.hh"
#include "video_driver.hh"
#include "i18n.hh"

// Screens
#include "screen_intro.hh"
#include "screen_songs.hh"
#include "screen_sing.hh"
#include "screen_practice.hh"
#include "screen_configuration.hh"
#include "screen_players.hh"
#include "screen_hiscore.hh"

#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/thread.hpp>
#include <csignal>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>

volatile bool g_quit = false;

bool g_take_screenshot = false;

extern "C" void quit(int) {
	g_quit = true;
}

/// can be thrown as an exception to quit the game
struct QuitNow {};

static void checkEvents_SDL(ScreenManager& sm, Window& window) {
	if (g_quit) {
		std::cout << "Terminating, please wait... (or kill the process)" << std::endl;
		throw QuitNow();
	}
	SDL_Event event;
	while(SDL_PollEvent(&event) == 1) {
		switch(event.type) {
		  case SDL_QUIT:
			sm.finished();
			break;
		  case SDL_VIDEORESIZE:
			window.resize(event.resize.w, event.resize.h);
			break;
		  case SDL_KEYDOWN:
			int keypressed  = event.key.keysym.sym;
			SDLMod modifier = event.key.keysym.mod;
			if (((keypressed == SDLK_RETURN || keypressed == SDLK_KP_ENTER) && modifier & KMOD_ALT) || keypressed == SDLK_F11) {
				config["graphic/fullscreen"].b() = !config["graphic/fullscreen"].b();
				continue; // Already handled here...
			}
			if (keypressed == SDLK_PRINT || keypressed == SDLK_F12) {
				g_take_screenshot = true;
				continue; // Already handled here...
			}
			if (keypressed == SDLK_F4 && modifier & KMOD_ALT) {
				sm.finished();
				continue; // Already handled here...
			}
			// Volume control
			if ((keypressed == SDLK_UP || keypressed == SDLK_DOWN) && modifier & KMOD_CTRL) {
				std::string curS = sm.getCurrentScreen()->getName();
				// Pick proper setting
				std::string which_vol = (curS == "Sing" || curS == "Practice")
				  ? "audio/music_volume" : "audio/preview_volume";
				// Adjust value
				if (keypressed == SDLK_UP) ++config[which_vol];
				else --config[which_vol];
				// Show message
				sm.flashMessage(config[which_vol].getShortDesc() + ": " + config[which_vol].getValue());
				continue; // Already handled here...
			}
			break;
		}
		// Forward to screen even if the input system takes it (ignoring pushEvent return value)
		// This is needed to allow navigation (quiting the song) to function even then
		input::SDL::pushEvent(event);
		sm.getCurrentScreen()->manageEvent(event);
		switch(glGetError()) {
			case GL_INVALID_ENUM: std::cerr << "OpenGL error: invalid enum" << std::endl; break;
			case GL_INVALID_VALUE: std::cerr << "OpenGL error: invalid value" << std::endl; break;
			case GL_INVALID_OPERATION: std::cerr << "OpenGL error: invalid operation" << std::endl; break;
			case GL_STACK_OVERFLOW: std::cerr << "OpenGL error: stack overflow" << std::endl; break;
			case GL_STACK_UNDERFLOW: std::cerr << "OpenGL error: stack underflow" << std::endl; break;
			case GL_OUT_OF_MEMORY: std::cerr << "OpenGL error: out of memory" << std::endl; break;
		}
	}
	if( config["graphic/fullscreen"].b() != window.getFullscreen() )
		window.setFullscreen(config["graphic/fullscreen"].b());
}

void mainLoop(std::string const& songlist) {
	Window window(config["graphic/window_width"].i(), config["graphic/window_height"].i(), config["graphic/fullscreen"].b());
	ScreenManager sm(window);
	try {
		sm.flashMessage(_("Audio playback..."), 0.0f, 1.0f, 1.0f); window.blank(); sm.drawFlashMessage(); window.swap();
		Audio audio;
		sm.flashMessage(_("Miscellaneous..."), 0.0f, 1.0f, 1.0f); window.blank(); sm.drawFlashMessage(); window.swap();
		Backgrounds backgrounds;
		Database database(getConfigDir() / "database.xml");
		Songs songs(database, songlist);
		boost::scoped_ptr<input::MidiDrums> midiDrums;
		// TODO: Proper error handling...
		try { midiDrums.reset(new input::MidiDrums); } catch (std::runtime_error&) {}
		sm.addScreen(new ScreenIntro("Intro", audio));
		sm.addScreen(new ScreenSongs("Songs", audio, songs, database));
		sm.addScreen(new ScreenSing("Sing", audio, database, backgrounds));
		sm.addScreen(new ScreenPractice("Practice", audio));
		sm.addScreen(new ScreenConfiguration("Configuration", audio));
		sm.addScreen(new ScreenPlayers("Players", audio, database));
		sm.addScreen(new ScreenHiscore("Hiscore", audio, songs, database));
		sm.activateScreen("Intro");
		sm.flashMessage(_("Main menu..."), 0.0f, 1.0f, 1.0f); window.blank(); sm.drawFlashMessage(); window.swap();
		sm.updateScreen();  // exit/enter, any exception is fatal error
		sm.flashMessage("");
		// Main loop
		boost::xtime time = now();
		unsigned frames = 0;
		while (!sm.isFinished()) {
			if( g_take_screenshot ) {
				fs::path filename;
				try {
					window.screenshot();
					sm.flashMessage(_("Screenshot taken!"));
				} catch (std::exception& e) {
					std::cerr << "ERROR: " << e.what() << std::endl;
					sm.flashMessage(_("Screenshot failed!"));
				}
				g_take_screenshot = false;
			}
			sm.updateScreen();  // exit/enter, any exception is fatal error
			try {
				// Draw
				window.blank();
				sm.getCurrentScreen()->draw();
				sm.drawFlashMessage();
				// Display (and wait until next frame)
				window.swap();
				if (config["graphic/fps"].b()) {
					++frames;
					if (now() - time > 1.0) {
						std::cout << frames << " FPS" << std::endl;
						time += 1.0;
						frames = 0;
					}
				} else {
					boost::thread::sleep(time + 0.01); // Max 100 FPS
					time = now();
					frames = 0;
				}
				// Process events for the next frame
				if (midiDrums) midiDrums->process();
				checkEvents_SDL(sm, window);
			} catch (std::runtime_error& e) {
				std::cerr << "ERROR: " << e.what() << std::endl;
				sm.flashMessage(std::string("ERROR: ") + e.what());
			}
		}
	} catch (std::exception& e) {
		std::cerr << "FATAL ERROR: " << e.what() << std::endl;
		sm.flashMessage(std::string("FATAL ERROR: ") + e.what(), 0.0f); // No fade-in to get it to show
		window.blank();
		sm.drawFlashMessage();
		window.swap();
		boost::thread::sleep(now() + 2.0);
	} catch (QuitNow&) {
		std::cout << "Terminated." << std::endl;
	}
}

/// Simple test utility to make mapping of joystick buttons/axes easier
void jstestLoop() {
	try {
		Window window(config["graphic/window_width"].i(), config["graphic/window_height"].i(), false);
		// Main loop
		boost::xtime time = now();
		int oldjoy = -1, oldaxis = -1, oldvalue = -1;
		while (true) {
			SDL_Event e;
			while(SDL_PollEvent(&e) == 1) {
				if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
					return;
				} else if (e.type == SDL_KEYDOWN) {
					std::cout << "Keyboard key: " << int(e.key.keysym.sym) << ", mod: " << int(e.key.keysym.mod) << std::endl;
				} else if (e.type == SDL_JOYBUTTONDOWN) {
					std::cout << "JoyID: " << int(e.jbutton.which) << ", button: " << int(e.jbutton.button) << ", state: " << int(e.jbutton.state) << std::endl;
				} else if (e.type == SDL_JOYAXISMOTION) {
					if ((oldjoy != int(e.jaxis.which)) || (oldaxis != int(e.jaxis.axis)) || (oldvalue != int(e.jaxis.value))) {
						std::cout << "JoyID: " << int(e.jaxis.which) << ", axis: " << int(e.jaxis.axis) << ", value: " << int(e.jaxis.value) << std::endl;
						oldjoy = int(e.jaxis.which);
						oldaxis = int(e.jaxis.axis);
						oldvalue = int(e.jaxis.value);
					}
				} else if (e.type == SDL_JOYHATMOTION) {
					std::cout << "JoyID: " << int(e.jhat.which) << ", hat: " << int(e.jhat.hat) << ", value: " << int(e.jhat.value) << std::endl;
				}
			}
			window.blank(); window.swap();
			boost::thread::sleep(time + 0.01); // Max 100 FPS
			time = now();
		}
	} catch (std::exception& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
	} catch (QuitNow&) {
		std::cout << "Terminated." << std::endl;
	}
	return;
}

template <typename Container> void confOverride(Container const& c, std::string const& name) {
	if (c.empty()) return;  // Don't override if no options specified
	ConfigItem::StringList& sl = config[name].sl();
	sl.clear();
	std::copy(c.begin(), c.end(), std::back_inserter(sl));
}

int main(int argc, char** argv) try {
#ifdef USE_GETTEXT
	// initialize gettext
#ifdef _MSC_VER
	setlocale(LC_ALL, "");//only untill we don't have a better solution. This because LC_MESSAGES cause crash under Visual Studio
#else
	setlocale (LC_MESSAGES, "");
#endif
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
#endif

	std::cout << PACKAGE " " VERSION << std::endl;
	std::signal(SIGINT, quit);
	std::signal(SIGTERM, quit);
	std::ios::sync_with_stdio(false);  // We do not use C stdio
	std::srand(std::time(NULL));
	// Parse commandline options
	std::vector<std::string> mics;
	std::vector<std::string> pdevs;
	std::vector<std::string> songdirs;
	namespace po = boost::program_options;
	po::options_description opt1("Generic options");
	std::string songlist;
	opt1.add_options()
	  ("help,h", "you are viewing it")
	  ("version,v", "display version number")
	  ("songlist", po::value<std::string>(&songlist), "save a list of songs in the specified folder");
	po::options_description opt2("Configuration options");
	opt2.add_options()
	  ("mics", po::value<std::vector<std::string> >(&mics)->composing(), "specify the microphones to use")
	  ("pdev", po::value<std::vector<std::string> >(&pdevs)->composing(), "specify the playback device")
	  ("michelp", "detailed help and device list for --mics")
	  ("pdevhelp", "detailed help and device list for --pdev")
	  ("jstest", "utility to get joystick button mappings");
	po::options_description opt3("Hidden options");
	opt3.add_options()
	  ("songdir", po::value<std::vector<std::string> >(&songdirs)->composing(), "");
	// Process flagless options as songdirs
	po::positional_options_description p;
	p.add("songdir", -1);
	po::options_description cmdline;
	cmdline.add(opt1).add(opt2);
	po::variables_map vm;
	// Load the arguments
	try {
		po::options_description allopts(cmdline);
		allopts.add(opt3);
		po::store(po::command_line_parser(argc, argv).options(allopts).positional(p).run(), vm);
	} catch (std::exception& e) {
		std::cout << cmdline << std::endl;
		std::cout << "ERROR: " << e.what() << std::endl;
		return 1;
	}
	po::notify(vm);
	if (vm.count("version")) {
		// Already printed the version string in the beginning...
		return 0;
	}
	if (vm.count("help")) {
		std::cout << cmdline << "  any arguments without a switch are interpreted as song folders.\n" << std::endl;
		return 0;
	}
#ifdef USE_PORTMIDI
	// Dump a list of MIDI input devices
	pm::dumpDevices(true);
#endif
	// Read config files
	try {
		readConfig();
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	// Override XML config for options that were specified from commandline or performous.conf
	confOverride(songdirs, "system/path_songs");
	confOverride(mics, "audio/capture");
	confOverride(pdevs, "audio/playback");
	getPaths(); // Initialize paths before other threads start
	if (vm.count("jstest")) { // Joystick test program
		std::cout << std::endl << "Joystick utility - Touch your joystick to see buttons here" << std::endl
		<< "Hit ESC (window focused) to quit" << std::endl << std::endl;
		jstestLoop();
		return 0;
	}
	// Run the game init and main loop
	mainLoop(songlist);
	return 0; // Do not remove. SDL_Main (which this function is called on some platforms) needs return statement.
} catch (std::exception& e) {
	std::cerr << "FATAL ERROR: " << e.what() << std::endl;
}

