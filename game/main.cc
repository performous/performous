#include "config.hh"
#include "downloader.hh"
#include "fs.hh"
#include "screen.hh"
#include "controllers.hh"
#include "profiler.hh"
#include "songs.hh"
#include "backgrounds.hh"
#include "database.hh"
#include "xtime.hh"
#include "video_driver.hh"
#include "i18n.hh"
#include "glutil.hh"
#include "log.hh"

// Screens
#include "screen_intro.hh"
#include "screen_songs.hh"
#include "screen_sing.hh"
#include "screen_practice.hh"
#include "screen_downloads.hh"
#include "screen_audiodevices.hh"
#include "screen_paths.hh"
#include "screen_players.hh"

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <csignal>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>

volatile bool g_quit = false;

bool g_take_screenshot = false;

// Signal handling for Ctrl-C

static void signalSetup();

extern "C" void quit(int) {
	// shouldn't "not EXIT_SUCCESS" be sent - ^C^C is an abort, not normal termination?
	if (g_quit) std::exit(EXIT_SUCCESS);  // Instant exit if Ctrl+C is pressed again
	g_quit = true;
	signalSetup();
}

static void signalSetup() {
	std::signal(SIGINT, quit);
	std::signal(SIGTERM, quit);
}

/// can be thrown as an exception to quit the game
struct QuitNow {};

static void checkEvents_SDL(ScreenManager& sm) {
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
			sm.window().resize(event.resize.w, event.resize.h);
			break;
		  case SDL_KEYDOWN:
			int keypressed  = event.key.keysym.sym;
			SDLMod modifier = event.key.keysym.mod;
			if (((keypressed == SDLK_RETURN || keypressed == SDLK_KP_ENTER) && modifier & KMOD_ALT) || keypressed == SDLK_F11) {
				config["graphic/fullscreen"].b() = !config["graphic/fullscreen"].b();
				continue; // Already handled here...
			}
			if (keypressed == SDLK_PRINT || (keypressed == SDLK_F12 && (modifier & KMOD_CTRL))) {
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
			// Eat away the event if there was a dialog open
			if (sm.closeDialog()) continue;
			break;
		}
		// Close dialog in case of a nav event
		if (sm.isDialogOpen() && input::getNav(event) != input::NONE) { sm.closeDialog(); continue; }
		// Forward to screen even if the input system takes it (ignoring pushEvent return value)
		// This is needed to allow navigation (quiting the song) to function even then
		boost::xtime eventTime = now();
		input::SDL::pushEvent(event, eventTime);
		sm.getCurrentScreen()->manageEvent(event);
	}
	if (config["graphic/fullscreen"].b() != sm.window().getFullscreen()) {
		sm.window().setFullscreen(config["graphic/fullscreen"].b());
		sm.reloadGL();
	}
}

void mainLoop(std::string const& songlist) {
	Audio audio;
	{ // Print the devices
		portaudio::AudioDevices ads;
		std::clog << "audio/info:\n" << ads.dump();
	}
	Downloader downloader;
	Window window(config["graphic/window_width"].i(), config["graphic/window_height"].i(), config["graphic/fullscreen"].b());
	Backgrounds backgrounds;
	Database database(getConfigDir() / "database.xml");
	Songs songs(database, songlist);
	ScreenManager sm(window);
	try {
		boost::scoped_ptr<input::MidiDrums> midiDrums;
		// TODO: Proper error handling...
		try { midiDrums.reset(new input::MidiDrums); } catch (std::runtime_error& e) {
			std::clog << "controllers/info: " << e.what() << std::endl;
		}
		// Load audio samples
		sm.loading(_("Loading audio samples..."), 0.5);
		audio.loadSample("drum bass", getPath("sounds/drum_bass.ogg"));
		audio.loadSample("drum snare", getPath("sounds/drum_snare.ogg"));
		audio.loadSample("drum hi-hat", getPath("sounds/drum_hi-hat.ogg"));
		audio.loadSample("drum tom1", getPath("sounds/drum_tom1.ogg"));
		audio.loadSample("drum cymbal", getPath("sounds/drum_cymbal.ogg"));
		//audio.loadSample("drum tom2", getPath("sounds/drum_tom2.ogg"));
		audio.loadSample("guitar fail1", getPath("sounds/guitar_fail1.ogg"));
		audio.loadSample("guitar fail2", getPath("sounds/guitar_fail2.ogg"));
		audio.loadSample("guitar fail3", getPath("sounds/guitar_fail3.ogg"));
		audio.loadSample("guitar fail4", getPath("sounds/guitar_fail4.ogg"));
		audio.loadSample("guitar fail5", getPath("sounds/guitar_fail5.ogg"));
		audio.loadSample("guitar fail6", getPath("sounds/guitar_fail6.ogg"));
		// Load screens
		sm.loading(_("Creating screens..."), 0.7);
		sm.addScreen(new ScreenIntro("Intro", audio));
		sm.addScreen(new ScreenSongs("Songs", audio, songs, database));
		sm.addScreen(new ScreenSing("Sing", audio, database, backgrounds));
		sm.addScreen(new ScreenPractice("Practice", audio));
		sm.addScreen(new ScreenDownloads("Downloads", audio, downloader));
		sm.addScreen(new ScreenAudioDevices("AudioDevices", audio));
		sm.addScreen(new ScreenPaths("Paths", audio));
		sm.addScreen(new ScreenPlayers("Players", audio, database));
		sm.activateScreen("Intro");
		sm.loading(_("Entering main menu"), 0.8);
		sm.updateScreen();  // exit/enter, any exception is fatal error
		sm.loading(_("Loading complete"), 1.0);
		// Main loop
		boost::xtime time = now();
		unsigned frames = 0;
		while (!sm.isFinished()) {
			Profiler prof("mainloop");
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
			prof("misc");
			try {
				// Draw
				window.render(boost::bind(&ScreenManager::drawScreen, &sm));
				glFinish();
				prof("draw");
				// Display (and wait until next frame)
				window.swap();
				glFinish();
				prof("swap");
				updateSurfaces();
				sm.prepareScreen();
				glFinish();
				prof("surfaces");
				if (config["graphic/fps"].b()) {
					++frames;
					if (now() - time > 1.0) {
						std::ostringstream oss;
						oss << frames << " FPS";
						sm.flashMessage(oss.str());
						time += 1.0;
						frames = 0;
					}
				} else {
					boost::thread::sleep(time + 0.01); // Max 100 FPS
					time = now();
					frames = 0;
				}
				prof("fpsctrl");
				// Process events for the next frame
				if (midiDrums) midiDrums->process();
				checkEvents_SDL(sm);
				prof("events");
			} catch (std::runtime_error& e) {
				std::cerr << "ERROR: " << e.what() << std::endl;
				sm.flashMessage(std::string("ERROR: ") + e.what());
			}
		}
	} catch (std::exception& e) {
		sm.fatalError(e.what());  // Notify the user
		throw;
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

void outputOptionalFeatureStatus();

int main(int argc, char** argv) try {
	// initialize gettext
	Gettext gettext(PACKAGE);

	std::cout << PACKAGE " " VERSION << std::endl;
	signalSetup();
	outputOptionalFeatureStatus();
	std::ios::sync_with_stdio(false);  // We do not use C stdio
	std::srand(std::time(NULL));
	// Parse commandline options
	std::vector<std::string> devices;
	std::vector<std::string> songdirs;
	namespace po = boost::program_options;
	po::options_description opt1("Generic options");
	std::string songlist;
	std::string loglevel_regexp;
	opt1.add_options()
	  ("help,h", "you are viewing it")
	  ("log,l", po::value<std::string>(&loglevel_regexp)->default_value(logger::default_log_level), "selects log level")
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
	} catch (std::exception& e) {
		std::cout << cmdline << std::endl;
		std::cout << "ERROR: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	po::notify(vm);

	// Initialize the verbose message sink
	//logger::__log_hh_test(); // debug
	logger::setup(loglevel_regexp);
	atexit(logger::teardown); // We might exit from many places due to audio hangs

	if (vm.count("version")) {
		// Already printed the version string in the beginning...
		return EXIT_SUCCESS;
	}
	if (vm.count("help")) {
		std::cout << cmdline << "  any arguments without a switch are interpreted as song folders.\n" << std::endl;
		return EXIT_SUCCESS;
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
	if (vm.count("audiohelp")) {
		Audio audio;
		// Print the devices
		portaudio::AudioDevices ads;
		std::cout << ads.dump();
		// Some examples
		std::cout << "Example --audio parameters" << std::endl;
		std::cout << "  --audio \"out=2\"         # Pick first working two-channel playback device" << std::endl;
		std::cout << "  --audio \"dev=1 out=2\"   # Pick device id 1 and assign stereo playback" << std::endl;
		std::cout << "  --audio 'dev=\"HDA Intel\" mics=blue,red'   # HDA Intel with two mics" << std::endl;
		std::cout << "  --audio 'dev=pulse out=2 mics=blue'       # PulseAudio with input and output" << std::endl;
		// Give audio a little time to shutdown but then just quit
		boost::thread audiokiller(boost::bind(&Audio::close, boost::ref(audio)));
		if (!audiokiller.timed_join(boost::posix_time::milliseconds(2000)))
			std::cout << "Audio hung." << std::endl;
		return EXIT_SUCCESS;
	}
	// Override XML config for options that were specified from commandline or performous.conf
	confOverride(songdirs, "paths/songs");
	confOverride(devices, "audio/devices");
	getPaths(); // Initialize paths before other threads start
	if (vm.count("jstest")) { // Joystick test program
		std::cout << std::endl << "Joystick utility - Touch your joystick to see buttons here" << std::endl
		<< "Hit ESC (window focused) to quit" << std::endl << std::endl;
		jstestLoop();
		return EXIT_SUCCESS;
	}

	// Run the game init and main loop
	mainLoop(songlist);

	return EXIT_SUCCESS; // Do not remove. SDL_Main (which this function is called on some platforms) needs return statement.
} catch (std::exception& e) {
	std::cerr << "FATAL ERROR: " << e.what() << std::endl;
	return EXIT_FAILURE;
}

void outputOptionalFeatureStatus() {
	std::cout    << "  Internationalization:   " <<
	(Gettext::enabled() ? "Enabled" : "Disabled")
	<< std::endl << "  MIDI I/O:               " <<
	(input::MidiDrums::enabled() ? "Enabled" : "Disabled")
	<< std::endl << "  Webcam support:         " <<
	(Webcam::enabled() ? "Enabled" : "Disabled")
	<< std::endl << "  Torrent support:         " <<
	(Downloader::enabled() ? "Enabled" : "Disabled")
	<< std::endl << std::endl;
}
