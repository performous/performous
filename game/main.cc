#include "config.hh"
#include "fs.hh"
#include "screen.hh"
#include "joystick.hh"
#include "screen_intro.hh"
#include "screen_songs.hh"
#include "screen_sing.hh"
#include "screen_practice.hh"
#include "screen_configuration.hh"
#include "screen_players.hh"
#include "screen_highscore.hh"
#include "video_driver.hh"
#include "xtime.hh"
#include <boost/format.hpp>
#include <boost/program_options.hpp>
// Needs at least Boost 1.36 and many systems don't have that: #include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/core.hpp>
#include <boost/thread.hpp>
#include <libda/audio.hpp>
#include <csignal>
#include <fstream>
#include <set>
#include <string>
#include <vector>

volatile bool g_quit = false;

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
	static bool esc = false;
	SDL_Event event;
	while(SDL_PollEvent(&event) == 1) {
		// catch input event first
		if(!input::SDL::pushEvent(event)) {
			switch(event.type) {
			  case SDL_QUIT:
				sm.finished();
				break;
			  case SDL_VIDEORESIZE:
				window.resize(event.resize.w, event.resize.h);
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
					config["graphic/fullscreen"].b() = !config["graphic/fullscreen"].b();
					continue; // Already handled here...
				}
				if (keypressed == SDLK_F4 && modifier & KMOD_ALT) {
					sm.finished();
					continue; // Already handled here...
				}
				break;
			}
			sm.getCurrentScreen()->manageEvent(event);
		}
		switch(glGetError()) {
			case GL_INVALID_ENUM: std::cerr << "OpenGL error: invalid enum" << std::endl; break;
			case GL_INVALID_VALUE: std::cerr << "OpenGL error: invalid value" << std::endl; break;
			case GL_INVALID_OPERATION: std::cerr << "OpenGL error: invalid operation" << std::endl; break;
			case GL_STACK_OVERFLOW: std::cerr << "OpenGL error: stack overflow" << std::endl; break;
			case GL_STACK_UNDERFLOW: std::cerr << "OpenGL error: stack underflow" << std::endl; break;
			case GL_OUT_OF_MEMORY: std::cerr << "OpenGL error: invalid enum" << std::endl; break;
		}
	}
	if( config["graphic/fullscreen"].b() != window.getFullscreen() )
		window.setFullscreen(config["graphic/fullscreen"].b());
}

void audioSetup(Capture& capture, Audio& audio) {
	// initialize audio argument parser
	using namespace boost::spirit; //::classic;
	unsigned channels, rate, frames;
	std::string devstr;
	// channel       ::= "channel=" integer
	// rate          ::= "rate=" integer
	// frame         ::= "frame=" integer
	// argument      ::= channel | rate | frame
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
		} catch (std::exception const& e) {
			std::cerr << "Playback device pdev=" << *it << " failed and will be ignored:\n  " << e.what() << std::endl;
		}
	}
	if (!audio.isOpen()) std::cerr << "No playback devices could be used. Please use --pdev to define one." << std::endl;
}

std::string songlist;

void mainLoop() {
	try {
		Capture capture;
		Audio audio;
		audioSetup(capture, audio);
		Songs songs(songlist);
		Players players(getSharePath("performous.xml"));
		ScreenManager sm;
		Window window(config["graphic/window_width"].i(), config["graphic/window_height"].i(), config["graphic/fullscreen"].b(), config["graphic/fs_width"].i(), config["graphic/fs_height"].i());
		sm.addScreen(new ScreenIntro("Intro", audio, capture));
		sm.addScreen(new ScreenSongs("Songs", audio, songs));
		sm.addScreen(new ScreenSing("Sing", audio, capture, players));
		sm.addScreen(new ScreenPractice("Practice", audio, capture));
		sm.addScreen(new ScreenConfiguration("Configuration", audio));
		sm.addScreen(new ScreenPlayers("Players", audio, players));
		sm.addScreen(new ScreenHiscore("Hiscore", audio, players));
		sm.activateScreen("Intro");
		// Main loop
		boost::xtime time = now();
		unsigned frames = 0;
		while (!sm.isFinished()) {
			checkEvents_SDL(sm, window);
			window.blank();
			sm.getCurrentScreen()->draw();
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
		}
	} catch (std::exception& e) {
		std::cout << "FATAL ERROR: " << e.what() << std::endl;
	} catch (QuitNow&) {
		std::cout << "Terminated." << std::endl;
	}
}

template <typename Container> void confOverride(Container const& c, std::string const& name) {
	if (c.empty()) return;  // Don't override if no options specified
	ConfigItem::StringList& sl = config[name].sl();
	sl.clear();
	std::copy(c.begin(), c.end(), std::back_inserter(sl));
}

int main(int argc, char** argv) {
	std::cout << PACKAGE " " VERSION << std::endl;
	std::signal(SIGINT, quit);
	std::signal(SIGTERM, quit);
	std::ios::sync_with_stdio(false);  // We do not use C stdio
	// Parse commandline options
	std::vector<std::string> mics;
	std::vector<std::string> pdevs;
	std::vector<std::string> songdirs;
	namespace po = boost::program_options;
	po::options_description opt1("Generic options");
	opt1.add_options()
	  ("help,h", "you are viewing it")
	  ("version,v", "display version number")
	  ("songlist", po::value<std::string>(&songlist), "save a list of songs in the specified folder");
	po::options_description opt2("Configuration options");
	opt2.add_options()
	  ("fs", po::value<bool>()->implicit_value(true), "start in full screen mode\n  --fs=0 for windowed mode")
	  ("width,W", po::value<int>(), "set horizontal resolution")
	  ("height,H", po::value<int>(), "set vertical resolution")
	  ("mics", po::value<std::vector<std::string> >(&mics)->composing(), "specify the microphones to use")
	  ("pdev", po::value<std::vector<std::string> >(&pdevs)->composing(), "specify the playback device")
	  ("michelp", "detailed help and device list for --mics")
	  ("pdevhelp", "detailed help and device list for --pdev")
	  ("theme", po::value<std::string>(), "set theme (name or absolute path)");
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
		return 0;
	}
	// Read config files
	try {
		readConfig();
		char const* env_data_dir = getenv("PERFORMOUS_DATA_DIR");
		if(env_data_dir) config["system/path_data"].sl().insert(config["system/path_data"].sl().begin(),std::string(env_data_dir));
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	// Override XML config for options that were specified from commandline or performous.conf
	if (vm.count("fs")) config["graphic/fullscreen"].b() = vm["fs"].as<bool>();
	std::string graphic = config["graphic/fullscreen"].b() ? "graphic/fs_" : "graphic/window_";
	if (vm.count("width")) config[graphic + "width"].i() = vm["width"].as<int>();
	if (vm.count("height")) config[graphic + "height"].i() = vm["height"].as<int>();
	confOverride(songdirs, "system/path_songs");
	confOverride(mics, "audio/capture");
	confOverride(pdevs, "audio/playback");
	// Run the game init and main loop
	mainLoop();
	return 0; // Do not remove. SDL_Main (which this function is called on some platforms) needs return statement.
}

