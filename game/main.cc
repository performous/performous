#include "backgrounds.hh"
#include "chrono.hh"
#include "config.hh"
#include "controllers.hh"
#include "database.hh"
#include "engine.hh"
#include "fs.hh"
#include "glutil.hh"
#include "i18n.hh"
#include "log.hh"
#include "platform.hh"
#include "profiler.hh"
#include "screen.hh"
#include "songs.hh"
#include "video_driver.hh"
#include "webcam.hh"
#include "webserver.hh"

// Screens
#include "screen_intro.hh"
#include "screen_songs.hh"
#include "screen_sing.hh"
#include "screen_practice.hh"
#include "screen_audiodevices.hh"
#include "screen_paths.hh"
#include "screen_players.hh"
#include "screen_playlist.hh"

#include <boost/program_options.hpp>
#include <cstdlib>
#include <csignal>
#include <string>
#include <thread>
#include <vector>

// Disable main level exception handling for debug builds (because gdb cannot properly catch throwing otherwise)
#ifdef NDEBUG
#define RUNTIME_ERROR std::runtime_error
#define EXCEPTION std::exception
#else
namespace { struct Nothing { char const* what() const { return nullptr; } }; }
#define RUNTIME_ERROR Nothing
#define EXCEPTION Nothing
#endif

std::atomic<bool> g_quit{ false };

bool g_take_screenshot = false;

// Signal handling for Ctrl-C

static void signalSetup();

extern "C" void quit(int) {
	using namespace std; // Apparently some implementations put quick_exit in std:: and others in ::
	if (g_quit) abort();  // Instant exit if Ctrl+C is pressed again
	g_quit = true;
	signalSetup();
}

static void signalSetup() {
	std::signal(SIGINT, quit);
	std::signal(SIGTERM, quit);
}

/// can be thrown as an exception to quit the game
struct QuitNow {};

static void checkEvents(Game& gm, Time eventTime) {
	if (g_quit) {
		std::cerr << "Terminating, please wait... (or kill the process)" << std::endl;
		throw QuitNow();
	}
	Window& window = gm.window();
	SDL_Event event;
	while (SDL_PollEvent(&event) == 1) {
		// Let the navigation system grab any and all SDL events
		gm.controllers.pushEvent(event, eventTime);
		auto type = event.type;
		if (type == SDL_WINDOWEVENT) window.event(event.window.event, event.window.data1, event.window.data2);
		if (type == SDL_QUIT) gm.finished();
		if (type == SDL_KEYDOWN) {
			auto key  = event.key.keysym.scancode;
			auto mod = event.key.keysym.mod;
			bool altEnter = (key == SDL_SCANCODE_RETURN || key == SDL_SCANCODE_KP_ENTER) && mod & KMOD_ALT;  // Alt+Enter
			bool modF = key == SDL_SCANCODE_F && mod & KMOD_CTRL && mod & KMOD_GUI;  // MacOS Ctrl+Cmd+F
			if (altEnter || modF || key == SDL_SCANCODE_F11) {
				config["graphic/fullscreen"].b() = !config["graphic/fullscreen"].b();
				continue; // Already handled here...
			}
			if (key == SDL_SCANCODE_PRINTSCREEN || (key == SDL_SCANCODE_F12 && (mod & Platform::shortcutModifier()))) {
				g_take_screenshot = true;
				continue; // Already handled here...
			}
			if (key == SDL_SCANCODE_F4 && mod & KMOD_ALT) {
				gm.finished();
				continue; // Already handled here...
			}
		}
		// Screens always receive SDL events that were not already handled here
		gm.getCurrentScreen()->manageEvent(event);
	}
	for (input::NavEvent event; gm.controllers.getNav(event); ) {
		input::NavButton nav = event.button;
		// Volume control
		if (nav == input::NAV_VOLUME_UP || nav == input::NAV_VOLUME_DOWN) {
			std::string curS = gm.getCurrentScreen()->getName();
			// Pick proper setting
			std::string which_vol = (curS == "Sing" || curS == "Practice")
			  ? "audio/music_volume" : "audio/preview_volume";
			// Adjust value
			if (nav == input::NAV_VOLUME_UP) ++config[which_vol]; else --config[which_vol];
			// Show message
			gm.flashMessage(config[which_vol].getShortDesc() + ": " + config[which_vol].getValue());
			continue; // Already handled here...
		}
		// If a dialog is open, any nav event will close it
		if (gm.isDialogOpen()) { gm.closeDialog(); }
		// Let the current screen handle other events
		gm.getCurrentScreen()->manageEvent(event);
	}

	// Need to toggle full screen mode or adjust resolution?
	window.resize();
}

void mainLoop(std::string const& songlist) {
	Platform platform;
	std::clog << "core/notice: Starting the audio subsystem (errors printed on console may be ignored)." << std::endl;
	Audio audio;
	std::clog << "core/info: Loading assets." << std::endl;
	TranslationEngine localization(PACKAGE);
	std::unique_ptr<Window> window;
	TextureLoader m_loader;
	Backgrounds backgrounds;
	Database database(getConfigDir() / "database.xml");
	Songs songs(database, songlist);
	loadFonts();
	try {
		window = std::make_unique<Window>();
		} catch (RUNTIME_ERROR& e) {
			std::cerr << "ERROR: " << e.what() << std::endl;
		}
	Game gm(*window, audio, songs);
	try {
		// Load audio samples
		gm.loading(_("Loading audio samples..."), 0.5);
		audio.loadSample("drum bass", findFile("sounds/drum_bass.ogg"));
		audio.loadSample("drum snare", findFile("sounds/drum_snare.ogg"));
		audio.loadSample("drum hi-hat", findFile("sounds/drum_hi-hat.ogg"));
		audio.loadSample("drum tom1", findFile("sounds/drum_tom1.ogg"));
		audio.loadSample("drum cymbal", findFile("sounds/drum_cymbal.ogg"));
		//audio.loadSample("drum tom2", findFile("sounds/drum_tom2.ogg"));
		audio.loadSample("guitar fail1", findFile("sounds/guitar_fail1.ogg"));
		audio.loadSample("guitar fail2", findFile("sounds/guitar_fail2.ogg"));
		audio.loadSample("guitar fail3", findFile("sounds/guitar_fail3.ogg"));
		audio.loadSample("guitar fail4", findFile("sounds/guitar_fail4.ogg"));
		audio.loadSample("guitar fail5", findFile("sounds/guitar_fail5.ogg"));
		audio.loadSample("guitar fail6", findFile("sounds/guitar_fail6.ogg"));
		audio.loadSample("notice.ogg",findFile("notice.ogg"));
		// Load screens
		gm.loading(_("Creating screens..."), 0.7);
		gm.addScreen(std::make_unique<ScreenIntro>("Intro", audio));
		gm.addScreen(std::make_unique<ScreenSongs>("Songs", audio, songs, database));
		gm.addScreen(std::make_unique<ScreenSing>("Sing", audio, database, backgrounds));
		gm.addScreen(std::make_unique<ScreenPractice>("Practice", audio));
		gm.addScreen(std::make_unique<ScreenAudioDevices>("AudioDevices", audio));
		gm.addScreen(std::make_unique<ScreenPaths>("Paths", audio, songs));
		gm.addScreen(std::make_unique<ScreenPlayers>("Players", audio, database));
		gm.addScreen(std::make_unique<ScreenPlaylist>("Playlist", audio, songs, backgrounds));
		gm.activateScreen("Intro");
		gm.loading(_("Entering main menu"), 0.8);
		gm.updateScreen();  // exit/enter, any exception is fatal error
		gm.loading(_("Loading complete"), 1.0);
		// Main loop
		auto time = Clock::now();
		unsigned frames = 0;
		std::clog << "core/info: Assets loaded, entering main loop." << std::endl;
		while (!gm.isFinished()) {
			Profiler prof("mainloop");
			bool benchmarking = config["graphic/fps"].b();
			if (songs.doneLoading == true && songs.displayedAlert == false) {
				gm.dialog(_("Done Loading!\n Loaded ") + std::to_string(songs.loadedSongs()) + " Songs.");
				songs.displayedAlert = true;
			}
			if (g_take_screenshot) {
				try {
					window->screenshot();
					gm.flashMessage(_("Screenshot taken!"));
				} catch (EXCEPTION& e) {
					std::cerr << "ERROR: " << e.what() << std::endl;
					gm.flashMessage(_("Screenshot failed!"));
				}
				g_take_screenshot = false;
			}
			gm.updateScreen();  // exit/enter, any exception is fatal error
			if (benchmarking) prof("misc");
			try {
				window->blank();
				// Draw
				window->render([&gm]{ gm.drawScreen(); });
				if (benchmarking) { glFinish(); prof("draw"); }
				// Display (and wait until next frame)
				window->swap();
				if (benchmarking) { glFinish(); prof("swap"); }
				updateTextures();
				gm.prepareScreen();
				if (benchmarking) { glFinish(); prof("textures"); }
				if (benchmarking) {
					++frames;
					if (Clock::now() - time > 1s) {
						std::ostringstream oss;
						oss << frames << " FPS";
						gm.flashMessage(oss.str());
						time += 1s;
						frames = 0;
					}
				} else {
					std::this_thread::sleep_until(time + 10ms); // Max 100 FPS
					time = Clock::now();
					frames = 0;
				}
				if (benchmarking) prof("fpsctrl");
				// Process events for the next frame
				auto eventTime = Clock::now();
				gm.controllers.process(eventTime);
				checkEvents(gm, eventTime);
				if (benchmarking) prof("events");
		} catch (RUNTIME_ERROR& e) {
			std::cerr << "ERROR: " << e.what() << std::endl;
			gm.flashMessage(std::string("ERROR: ") + e.what());
			}
		}
		writeConfig();
	} catch (EXCEPTION& e) {
		std::clog << "core/error: Exiting due to fatal error: " << e.what() << std::endl;
		gm.fatalError(e.what());  // Notify the user
		throw;
		} catch (QuitNow&) {
		std::cerr << "Terminated." << std::endl;
		}
}

/// Simple test utility to make mapping of joystick buttons/axes easier
void jstestLoop() {
	try {
		config["graphic/fullscreen"].b() = false;
		config["graphic/window_width"].i() = 640;
		config["graphic/window_height"].i() = 360;
		Window window;
		// Main loop
		int oldjoy = -1, oldaxis = -1, oldvalue = -1;
		while (true) {
			SDL_Event e;
			while(SDL_PollEvent(&e) == 1) {
				if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
					return;
				} else if (e.type == SDL_KEYDOWN) {
					std::cout << "Keyboard key: " << int(e.key.keysym.scancode) << ", mod: " << int(e.key.keysym.mod) << std::endl;
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
			std::this_thread::sleep_for(10ms); // Max 100 FPS
		}
	} catch (EXCEPTION& e) {
		std::cerr << "ERROR: " << e.what() << std::endl;
	} catch (QuitNow&) {
		std::cerr << "Terminated." << std::endl;
	}
	return;
}

template <typename Container> void confOverride(Container const& c, std::string const& name) {
	if (c.empty()) return;  // Don't override if no options specified
	ConfigItem::StringList& sl = config[name].sl();
	sl.clear();
	std::copy(c.begin(), c.end(), std::back_inserter(sl));
}

void outputOptionalFeatureStatus();

void fatalError(std::string msg, bool hasLog = false, std::string title = "FATAL ERROR") {
	std::ostringstream errMsg;
	errMsg << msg;
	if (hasLog) {
		errMsg << std::endl << "More details might be available in " << getLogFilename() << ".";
	}
	errMsg << std::endl << "If you think this is a bug in Performous, please report it at "
	  << std::endl << "  https://github.com/performous/performous/issues";
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(),
	  errMsg.str().c_str(), nullptr);
	std::cerr << title << ": " << msg << std::endl;
	if (hasLog) {
		std::clog << "core/error: " << errMsg.str() << std::endl;
	}
}

int main(int argc, char** argv) try {
	signalSetup();
	std::srand(std::time(nullptr));
	// Parse commandline options
	std::vector<std::string> devices;
	std::vector<std::string> songdirs;
	namespace po = boost::program_options;
	po::options_description opt1("Generic options");
	std::string songlist;
	std::string loglevel;
	opt1.add_options()
	  ("help,h", "you are viewing it")
	  ("log,l", po::value<std::string>(&loglevel), "subsystem name or minimum level to log")
	  ("version,v", "display version number")
	  ("songlist", po::value<std::string>(&songlist), "save a list of songs in the specified folder");
	po::options_description opt2("Configuration options");
	opt2.add_options()
	  ("audio", po::value<std::vector<std::string> >(&devices)->composing(), "specify an audio device to use")
	  ("audiohelp", "print audio related information")
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
	} catch (EXCEPTION& e) {
		std::cerr << cmdline << std::endl;
		std::cerr << "ERROR: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	po::notify(vm);

	if (vm.count("version")) {
		std::cout << PACKAGE " " VERSION << std::endl;
		return EXIT_SUCCESS;
	}
	if (vm.count("help")) {
		std::cout << cmdline << "  any arguments without a switch are interpreted as song folders.\n" << std::endl;
		return EXIT_SUCCESS;
	}

	Logger logger(loglevel);
	try {
		outputOptionalFeatureStatus();

		// Read config files
		readConfig();

		if (vm.count("audiohelp")) {
			std::clog << "core/notice: Starting audio subsystem for audiohelp (errors printed on console may be ignored)." << std::endl;
			Audio audio;
			// Print the devices
			std::cout << portaudio::AudioBackends().dump();
			// Some examples
			std::cout << "Example --audio parameters" << std::endl;
			std::cout << "  --audio \"out=2\"         # Pick first working two-channel playback device" << std::endl;
			std::cout << "  --audio \"dev=1 out=2\"   # Pick device id 1 and assign stereo playback" << std::endl;
			std::cout << "  --audio 'dev=\"HDA Intel\" mics=blue,red'   # HDA Intel with two mics" << std::endl;
			std::cout << "  --audio 'dev=pulse out=2 mics=blue'       # PulseAudio with input and output" << std::endl;
			return EXIT_SUCCESS;
		}
		// Override XML config for options that were specified from commandline or performous.conf
		confOverride(songdirs, "paths/songs");
		confOverride(devices, "audio/devices");
		getPaths(); // Initialize paths before other threads start
		if (vm.count("jstest")) { // Joystick test program
			std::clog << "core/notice: Starting jstest input test utility." << std::endl;
			std::cout << std::endl << "Joystick utility - Touch your joystick to see buttons here" << std::endl
			<< "Hit ESC (window focused) to quit" << std::endl << std::endl;
			jstestLoop();
			return EXIT_SUCCESS;
		}
		// Run the game init and main loop
		mainLoop(songlist);

		return EXIT_SUCCESS; // Do not remove. SDL_Main (which this function is called on some platforms) needs return statement.
	} catch (EXCEPTION& e) {
		// After logging is initialized, we can also inform the user about the log file.
		fatalError(e.what(), true);
		return EXIT_FAILURE;
	}
} catch (EXCEPTION& e) {
	fatalError(e.what());
	return EXIT_FAILURE;
}

void outputOptionalFeatureStatus() {
	std::clog << "core/notice: " PACKAGE " " VERSION " starting..."
	  << "\n  Internationalization: " << (TranslationEngine::enabled() ? "Enabled" : "Disabled")
	  << "\n  MIDI Hardware I/O:    " << (input::Hardware::midiEnabled() ? "Enabled" : "Disabled")
	  << "\n  Webcam support:       " << (Webcam::enabled() ? "Enabled" : "Disabled")
	  << std::endl;
}
