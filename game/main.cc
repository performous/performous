#include "config.hh"
#include <audio.hpp>
#include "screen.hh"
#include "screen_intro.hh"
#include "screen_songs.hh"
#include "screen_sing.hh"
#include "screen_practice.hh"
#include "screen_configuration.hh"
#include "video_driver.hh"
#include "xtime.hh"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/spirit/core.hpp>
#include <boost/thread.hpp>
#include <fstream>
#include <set>
#include <string>
#include <vector>

namespace fs = boost::filesystem;

SDL_Surface * screenSDL;

volatile bool g_quit = false;

extern "C" void quit(int) {
	g_quit = true;
}

struct QuitNow {};

static void checkEvents_SDL(CScreenManager& sm, Window& window) {
	if (g_quit) {
		std::cout << "Terminating, please wait... (or kill the process)" << std::endl;
		throw QuitNow();
	}
	static bool esc = false;
	SDL_Event event;
	while(SDL_PollEvent(&event) == 1) {
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
				window.fullscreen();
				continue; // Already handled here...
			}
			break;
		}
		sm.getCurrentScreen()->manageEvent(event);
		GLenum error = glGetError();
		if (error == GL_INVALID_ENUM) std::cerr << "OpenGL error: invalid enum" << std::endl;
		if (error == GL_INVALID_VALUE) std::cerr << "OpenGL error: invalid value" << std::endl;
		if (error == GL_INVALID_OPERATION) std::cerr << "OpenGL error: invalid operation" << std::endl;
		if (error == GL_STACK_OVERFLOW) std::cerr << "OpenGL error: stack overflow" << std::endl;
		if (error == GL_STACK_UNDERFLOW) std::cerr << "OpenGL error: stack underflow" << std::endl;
		if (error == GL_OUT_OF_MEMORY) std::cerr << "OpenGL error: invalid enum" << std::endl;
	}
}

#include <signal.h>

int main(int argc, char** argv) {
	signal(SIGINT, quit);
	signal(SIGQUIT, quit);
	signal(SIGTERM, quit);
	std::ios::sync_with_stdio(false);  // We do not use C stdio
	da::initialize libda;
	bool fullscreen = false;
	bool fps = false;
	unsigned int width, height;
	std::string songlist;
	std::string theme;
	std::set<std::string> songdirs;
	std::vector<std::string> mics;
	std::vector<std::string> pdevs;
	std::string homedir;
	{
		char const* home = getenv("HOME");
		if (home) homedir = std::string(home) + '/';
		readConfigfile( homedir + ".config/performous.xml");
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
		  ("songlist", po::value<std::string>(&songlist), "save a list of songs in the specified folder")
		  ("width,W", po::value<unsigned int>(&width)->default_value(800), "set horizontal resolution")
		  ("height,H", po::value<unsigned int>(&height)->default_value(600), "set vertical resolution")
		  ("michelp", "detailed help for --mics and a list of available audio devices")
		  ("mics", po::value<std::vector<std::string> >(&mics)->composing(), "specify microphones to use")
		  ("pdevhelp", "detailed help for --pdev and a list of available audio devices")
		  ("pdev", po::value<std::vector<std::string> >(&pdevs)->composing(), "specify a playback device")
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
				std::ifstream conf((homedir + ".config/performous.conf").c_str());
				po::store(po::parse_config_file(conf, opt2), vm);
			}
			{
				std::ifstream conf("/etc/performous.conf");
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
			  "are be assigned as players until the maximum of four players are configured.\n" << std::endl;
			std::cout << "Capture devices available:" << std::endl;
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
			if (!homedir.empty()) songdirs.insert(homedir + ".ultrastar/songs/");
			songdirs.insert("/usr/local/share/games/ultrastar/songs/");
			songdirs.insert("/usr/share/games/ultrastar/songs/");
		}
		// Figure out theme folder
		if (theme.find('/') == std::string::npos) {
			char const* envthemepath = getenv("PERFORMOUS_THEME_PATH");
			std::string themepath;
			if (envthemepath) themepath = envthemepath;
			else themepath = "/usr/local/share/games/performous/themes:/usr/share/games/performous/themes";
			std::istringstream iss(themepath);
			std::string elem;
			while (std::getline(iss, elem, ':')) {
				if (elem.empty()) continue;
				fs::path p = elem;
				p /= theme;
				if (fs::is_directory(p)) { theme = p.string(); break; }
			}
        }
		if (*theme.rbegin() == '/') theme.erase(theme.size() - 1); // Remove trailing slash
	}
	// Built-in defaults:
	if( mics.empty() ) {
		mics.push_back("alsa:hw:default"); // Singstar mics
		mics.push_back(""); // Anything goes
	}
	if( pdevs.empty() ) {
		pdevs.push_back(""); // Anything goes
	}
	try {
		Capture capture;
		Audio audio;
		{
			// initialize audio argument parser
			using namespace boost::spirit;
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
			for(std::size_t i = 0; i < mics.size(); ++i) {
				channels = 2; rate = 48000; frames = 256; devstr.clear();
				if (!parse(mics[i].c_str(), device).full) throw std::runtime_error("Invalid syntax in mics=" + mics[i]);
				try {
					capture.addMics(channels, rate, devstr);
				} catch (std::exception const& e) {
					std::cerr << "Capture device mics=" << mics[i] << " failed and will be ignored:\n  " << e.what() << std::endl;
				}
			}
			if (capture.analyzers().empty()) std::cerr << "No capture devices could be used. Please use --mics to define some." << std::endl;
			// Playback devices
			for(std::size_t i = 0; i < pdevs.size() && !audio.isOpen(); ++i) {
				channels = 2; rate = 48000; frames = 256; devstr.clear();
				if (!parse(pdevs[i].c_str(), device).full) throw std::runtime_error("Invalid syntax in pdev=" + pdevs[i]);
				if (channels != 2) throw std::runtime_error("Only stereo playback is supported, error in pdev=" + pdevs[i]);
				try {
					audio.open(devstr, rate);
				} catch (std::exception const& e) {
					std::cerr << "Playback device pdev=" << pdevs[i] << " failed and will be ignored:\n  " << e.what() << std::endl;
				}
			}
			if (!audio.isOpen()) std::cerr << "No playback devices could be used. Please use --pdev to define one." << std::endl;
		}
		Songs songs(songdirs, songlist);
		CScreenManager sm(theme);
		Window window(width, height, fullscreen);
		sm.addScreen(new CScreenIntro("Intro", audio, capture));
		sm.addScreen(new CScreenSongs("Songs", audio, songs));
		sm.addScreen(new CScreenSing("Sing", audio, songs, capture));
		sm.addScreen(new CScreenPractice("Practice", audio, capture));
		sm.addScreen(new CScreenConfiguration("Configuration", audio));
		sm.activateScreen("Intro");
		// Main loop
		boost::xtime time = now();
		unsigned nb_frames = 0;
		while (!sm.isFinished()) {
			checkEvents_SDL(sm, window);
			window.blank();
			sm.getCurrentScreen()->draw();
			window.swap();
			++nb_frames;
			if (fps) {
				if (now() - time > 1.0) {
					std::cout << nb_frames << " FPS" << std::endl;
					time += 1.0;
					nb_frames = 0;
				}
			} else {
				boost::thread::sleep(time + 0.01); // Max 100 FPS
				time = now();
			}
		}
	} catch (std::exception& e) {
		std::cout << "FATAL ERROR: " << e.what() << std::endl;
	} catch (QuitNow&) {
		std::cout << "Terminated." << std::endl;
	}
	return 0; // Do not remove. SDL_Main (which this function is called on some platforms) needs return statement.
}

