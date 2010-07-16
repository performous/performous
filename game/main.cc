#include "config.hh"
#include "fs.hh"
#include "record.hh"
#include "screen.hh"
#include "joystick.hh"
#include "songs.hh"
#include "backgrounds.hh"
#include "database.hh"
#include "xtime.hh"
#include "video_driver.hh"
#include "i18n.hh"
#include "glutil.hh"

// Screens
#include "screen_intro.hh"
#include "screen_songs.hh"
#include "screen_sing.hh"
#include "screen_practice.hh"
#include "screen_configuration.hh"
#include "screen_players.hh"

#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/thread.hpp>
#include <libda/audio.hpp>
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
			break;
		}
		// Forward to screen even if the input system takes it (ignoring pushEvent return value)
		// This is needed to allow navigation (quiting the song) to function even then
		input::SDL::pushEvent(event);
		sm.getCurrentScreen()->manageEvent(event);
		// Check for OpenGL errors
		glutil::GLErrorChecker glerror;
	}
	if( config["graphic/fullscreen"].b() != sm.window().getFullscreen() )
		sm.window().setFullscreen(config["graphic/fullscreen"].b());
}

void audioSetup(Capture& capture, Audio& audio) {
	// initialize audio argument parser
	using namespace boost::spirit::classic;
	unsigned channels, rate, frames;
	std::string devstr;
	// channels      ::= "channels=" integer
	// rate          ::= "rate=" integer
	// frames        ::= "frames=" integer
	// argument      ::= channels | rate | frames
	// argument_list ::= integer? argument % ","
	// backend       ::= anychar+
	// device        ::= argument_list "@" backend | argument_list | backend
	rule<> channels_r = ("channels=" >> uint_p[assign_a(channels)]) | uint_p[assign_a(channels)];
	rule<> rate_r = "rate=" >> uint_p[assign_a(rate)];
	rule<> frames_r = "frames=" >> uint_p[assign_a(frames)];
	rule<> argument = channels_r | rate_r | frames_r;
	rule<> argument_list = argument % ',';
	rule<> device = (!argument_list >> '@' >> (+anychar_p)[assign_a(devstr)]) | argument_list | (*~ch_p('@'))[assign_a(devstr)];
	// Capture devices
	ConfigItem::StringList const& cdevs = config["audio/capture"].sl();
	for (ConfigItem::StringList::const_iterator it = cdevs.begin(); it != cdevs.end(); ++it) {
		channels = 2; rate = 48000; frames = 512; devstr.clear();
		if (!parse(it->c_str(), device).full) throw std::runtime_error("Invalid syntax in mics=" + *it);
		try {
			capture.addMics(channels, rate, frames, devstr);
		} catch (std::exception const& e) {
			std::cerr << "Capture device mics=" << *it << " failed and will be ignored:\n  " << e.what() << std::endl;
		}
	}
	if (capture.analyzers().empty()) std::cerr << "No capture devices could be used. Please use --mics to define some." << std::endl;
	// Playback devices
	ConfigItem::StringList const& pdevs = config["audio/playback"].sl();
	for (ConfigItem::StringList::const_iterator it = pdevs.begin(); it != pdevs.end(); ++it) {
		channels = 2; rate = 48000; frames = 512; devstr.clear();
		if (!parse(it->c_str(), device).full) throw std::runtime_error("Invalid syntax in pdev=" + *it);
		if (channels != 2) throw std::runtime_error("Only stereo playback is supported, error in pdev=" + *it);
		try {
			audio.open(devstr, rate, frames);
			// when we get here, we have successfully opened a device.
			// let's use it then!
			break;
		} catch (std::exception const& e) {
			std::cerr << "Playback device pdev=" << *it << " failed and will be ignored:\n  " << e.what() << std::endl;
		}
	}
	if (!audio.isOpen()) std::cerr << "No playback devices could be used. Please use --pdev to define one." << std::endl;
}

void mainLoop(std::string const& songlist) {
	Window window(config["graphic/window_width"].i(), config["graphic/window_height"].i(), config["graphic/fullscreen"].b());
	ScreenManager sm(window);
	try {
		sm.flashMessage(_("Audio capture..."), 0.0f, 1.0f, 1.0f); window.blank(); sm.drawFlashMessage(); window.swap();
		Capture capture;
		sm.flashMessage(_("Audio playback..."), 0.0f, 1.0f, 1.0f); window.blank(); sm.drawFlashMessage(); window.swap();
		Audio audio;
		audioSetup(capture, audio);
		sm.flashMessage(_("Miscellaneous..."), 0.0f, 1.0f, 1.0f); window.blank(); sm.drawFlashMessage(); window.swap();
		Backgrounds backgrounds;
		Database database(getConfigDir() / "database.xml");
		Songs songs(database, songlist);
		boost::scoped_ptr<input::MidiDrums> midiDrums;
		// TODO: Proper error handling...
		try { midiDrums.reset(new input::MidiDrums); } catch (std::runtime_error&) {}
		sm.addScreen(new ScreenIntro("Intro", audio, capture));
		sm.addScreen(new ScreenSongs("Songs", audio, songs, database));
		sm.addScreen(new ScreenSing("Sing", audio, capture, database, backgrounds));
		sm.addScreen(new ScreenPractice("Practice", audio, capture));
		sm.addScreen(new ScreenConfiguration("Configuration", audio));
		sm.addScreen(new ScreenPlayers("Players", audio, database));
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
				checkEvents_SDL(sm);
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
	// Initialize audio for pdevhelp, michelp and the rest of the program
	da::initialize libda;
	if (vm.count("pdevhelp")) {
		da::playback::devlist_t l = da::playback::devices();
		std::cout << "Specify with --pdev [OPTIONS@]dev[:settings]]. For example:\n"
		  "  --pdev jack                       JACK output\n"
		  "  --pdev rate=44100,frames=512@alsa ALSA with 44.1 kHz and buffers of 512 samples\n"
		  "                                    Nearest available rate will be used.\n"
		  "  --pdev alsa:hw:Intel              Use ALSA sound card named Intel.\n\n"
		  "If multiple pdevs are specified, they will all be tested to find a working one.\n" << std::endl;
		std::cout << "Playback devices available:" << std::endl;
		for (da::playback::devlist_t::const_iterator it = l.begin(); it != l.end(); ++it) {
			std::cout << boost::format("  %1% %|10t|%2%\n") % it->name() % it->desc();
		}
		std::cout << std::flush;
		return 0;
	}
	if (vm.count("michelp")) {
		da::record::devlist_t l = da::record::devices();
		std::cout << "Specify with --mics [OPTIONS@]dev[:settings]]. For example:\n"
		  "  --mics channels=2                 Two mics on any sound device\n"
		  "  --mics channels=2@jack            Two mics on JACK\n"
		  "  --mics channels=18@alsa:hw:M16DX  18 input channels on ALSA device \"hw:M16DX\"\n"
		  "                                    Note: only the first four at most will be used\n"
		  "  --mics channels=1,rate=44100      One mic; will try to get 44100 Hz if available\n\n"
		  "Multiple --mics options may be specified and all the successfully opened inputs\n"
		  "are assigned as players until the maximum of four players are configured.\n" << std::endl;
		std::cout << "Capture devices available:" << std::endl;
		for (da::record::devlist_t::const_iterator it = l.begin(); it != l.end(); ++it) {
			std::cout << boost::format("  %1% %|10t|%2%\n") % it->name() % it->desc();
		}
		std::cout << std::flush;
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

void outputOptionalFeatureStatus() {
	std::cout    << "  Internationalization:   " <<
	(Gettext::enabled() ? "Enabled" : "Disabled")
	<< std::endl << "  MIDI I/O:               " <<
	(input::MidiDrums::enabled() ? "Enabled" : "Disabled")
	<< std::endl << "  Webcam support:         " <<
	(Webcam::enabled() ? "Enabled" : "Disabled")
	<< std::endl;
}
