#include "screen_sing.hh"

#include "backgrounds.hh"
#include "dancegraph.hh"
#include "database.hh"
#include "engine.hh"
#include "fs.hh"
#include "glutil.hh"
#include "guitargraph.hh"
#include "i18n.hh"
#include "layout_singer.hh"
#include "menu.hh"
#include "platform.hh"
#include "screen_players.hh"
#include "songparser.hh"
#include "util.hh"
#include "video.hh"
#include "webcam.hh"
#include "screen_songs.hh"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <utility>

namespace {
	static const double QUIT_TIMEOUT = 20.0; // Return to songs screen after 20 seconds in score screen

	/// Add a flash message about the state of a config item
	void dispInFlash(ConfigItem& ci) {
		Game* gm = Game::getSingletonPtr();
		gm->flashMessage(ci.getShortDesc() + ": " + ci.getValue());
	}
}

ScreenSing::ScreenSing(std::string const& name, Audio& audio, Database& database, Backgrounds& bgs):
	Screen(name), m_audio(audio), m_database(database), m_backgrounds(bgs),
	m_selectedTrack(TrackName::LEAD_VOCAL)
{}

void ScreenSing::enter() {
	keyPressed = false;
	m_DuetTimeout.setValue(10);
	Game* gm = Game::getSingletonPtr();
	// Initialize webcam
	gm->loading(_("Initializing webcam..."), 0.1);
	if (config["graphic/webcam"].b() && Webcam::enabled()) {
		try {
			m_cam.reset(new Webcam(config["graphic/webcamid"].i()));
		} catch (std::exception& e) { std::cout << e.what() << std::endl; };
	}
	// Load video
	gm->loading(_("Loading video..."), 0.2);
	if (!m_song->video.empty() && config["graphic/video"].b()) {
		m_video.reset(new Video(m_song->video, m_song->videoGap));
	}
	boost::ptr_vector<Analyzer>& analyzers = m_audio.analyzers();
	reloadGL();
	// Load song notes
	gm->loading(_("Loading song..."), 0.4);
	try { m_song->loadNotes(false /* don't ignore errors */); }
	catch (SongParserException& e) {
		std::clog << e;
		gm->activateScreen("Songs");
	}
	// Notify about broken tracks
	if (!m_song->b0rked.empty()) gm->dialog(_("Song contains broken tracks!") + std::string("\n\n") + m_song->b0rked);
	// Startup delay for instruments is longer than for singing only
	double setup_delay = (!m_song->hasControllers() ? -1.0 : -5.0);
	m_audio.pause();
	m_audio.playMusic(m_song->music, false, 0.0, setup_delay);
	gm->loading(_("Loading menu..."), 0.7);
	{
		VocalTracks const& tracks = m_song->vocalTracks;
		unsigned players = analyzers.size();
		m_menu.clear();
		m_menu.add(MenuOption(_("Start"), _("Start performing")).call(std::bind(&ScreenSing::setupVocals, this)));
		m_duet = ConfigItem(0);
		if (players == 0) players = 1;  // No mic? We display lyrics anyway
		if (players > 1) { // Duet toggle
			m_duet.addEnum(_("Duet mode"));
			m_duet.addEnum(_("Normal mode"));
			m_menu.add(MenuOption("", _("Switch between duet and regular singing mode")).changer(m_duet));
		}
		// Add vocal track selector for each player
		for (unsigned player = 0; player < players; ++player) {
			ConfigItem& vocalTrack = m_vocalTracks[player];
			vocalTrack = ConfigItem(0);
			for (auto const& track: tracks) vocalTrack.addEnum(track.second.name);
			if (tracks.size() > 1 && player % 2) ++vocalTrack;  // Every other player gets the second track
			m_menu.add(MenuOption("", _("Change vocal track")).changer(vocalTrack));
		}
		m_menu.add(MenuOption(_("Quit"), _("Exit to song browser")).screen("Songs"));
		m_menu.open();
		if (tracks.size() <= 1) setupVocals();  // No duet menu
	}
	gm->showLogo(false);
	gm->loading(_("Loading complete"), 1.0);
}

void ScreenSing::setupVocals() {
	if (!m_song->vocalTracks.empty()) {
		m_layout_singer.clear();
		Engine::VocalTrackPtrs selectedTracks;
		boost::ptr_vector<Analyzer>& analyzers = m_audio.analyzers();
		unsigned players = (analyzers.empty() ? 1 : analyzers.size());  // Always at least 1; should be number of mics
		std::set<VocalTrack*> shownTracks;  // Tracks to be included in layout_singer (stored by name for proper sorting and merging duplicates)
		for (unsigned player = 0; player < players; ++player) {
			VocalTrack* vocal = &m_song->getVocalTrack(m_vocalTracks[(m_duet.i() == 0 ? player : 0)].i());
			selectedTracks.push_back(vocal);
			shownTracks.insert(vocal);
		}
		//if (shownTracks.size() > 2) throw std::runtime_error("Too many tracks chosen. Only two vocal tracks can be used simultaneously.");
		for (auto const& trk: shownTracks) m_layout_singer.push_back(new LayoutSinger(*trk, m_database, theme));
		// Note: Engine maps tracks with analyzers 1:1. If user doesn't have mics, we still want to have singer layout enabled but without engine...
		if (!analyzers.empty()) m_engine.reset(new Engine(m_audio, selectedTracks, m_database));
	}
	createPauseMenu();
	m_audio.pause(false);
}

void ScreenSing::createPauseMenu() {
	m_menu.clear();
	m_menu.add(MenuOption(_("Resume"), _("Back to performing!")));
	m_menu.add(MenuOption(_("Restart"), _("Start the song\nfrom the beginning")).screen("Sing"));
	Game* gm = Game::getSingletonPtr();
	if(!gm->getCurrentPlayList().isEmpty() || config["game/autoplay"].b()){
		m_menu.add(MenuOption(_("Skip"), _("Skip current song")).screen("Playlist"));
	}
	m_menu.add(MenuOption(_("Quit"), _("Exit to song browser")).call([]() {
		Game* gm = Game::getSingletonPtr();
		gm->activateScreen("Songs");
	}));
	m_menu.close();
}

void ScreenSing::reloadGL() {
	// Load UI graphics
	theme.reset(new ThemeSing());
	m_menuTheme.reset(new ThemeInstrumentMenu());
	m_pause_icon.reset(new Surface(findFile("sing_pause.svg")));
	m_player_icon.reset(new Surface(findFile("sing_pbox.svg"))); // For duet menu
	m_help.reset(new Surface(findFile("instrumenthelp.svg")));
	m_progress.reset(new ProgressBar(findFile("sing_progressbg.svg"), findFile("sing_progressfg.svg"), ProgressBar::HORIZONTAL, 0.01f, 0.01f, true));
	// Load background
	if (!m_song->background.empty()) m_background.reset(new Surface(m_song->background));
}

void ScreenSing::exit() {
    Game::getSingletonPtr()->controllers.enableEvents(false);
	m_engine.reset();
	m_score_window.reset();
	m_menu.clear();
	m_instruments.clear();
	m_layout_singer.clear();
	m_help.reset();
	m_pause_icon.reset();
	m_player_icon.reset();
	m_cam.reset();
	m_video.reset();
	m_background.reset();
	m_song->dropNotes();
	m_menuTheme.reset();
	theme.reset();
    m_audio.fadeout(0);
	if (m_audio.isPaused()) m_audio.togglePause();
	Game::getSingletonPtr()->showLogo();
}



/// Manages the instrument drawing
void ScreenSing::instrumentLayout(double time) {
	if (!m_song->hasControllers()) return;
	int count_alive = 0, count_menu = 0, i = 0;
	// Remove dead instruments and do the counting
	for (Instruments::iterator it = m_instruments.begin(); it != m_instruments.end(); ) {
		if (it->dead()) {
			it = m_instruments.erase(it);
			continue;
		}
		++count_alive;
		if (it->menuOpen()) ++count_menu;
		++it;
	}
	if (count_alive > 0) {
		// Handle pause
		bool shouldPause = count_menu > 0 || m_menu.isOpen();
		if (shouldPause != m_audio.isPaused()) m_audio.togglePause();
	} else if (time < -0.5) {
		// Display help if no-one has joined yet
		ColorTrans c(Color::alpha(clamp(-1.0 - 2.0 * time)));
		m_help->draw();
	}
	double iw = std::min(0.5, 1.0 / count_alive);
	typedef std::pair<unsigned, double> CountSum;
	std::map<std::string, CountSum> volume; // Stream id to (count, sum)
	std::map<std::string, CountSum> pitchbend; // Stream id to (count, sum)
	for (Instruments::iterator it = m_instruments.begin(); it != m_instruments.end(); ++it, ++i) {
		it->engine();
		it->position((0.5 + i - 0.5 * count_alive) * iw, iw); // Do layout stuff
		it->draw(time);
		{
			CountSum& cs = volume[it->getTrack()];
			cs.first++;
			cs.second += it->correctness();
		}{
			CountSum& cs = pitchbend[it->getTrack()];
			cs.first++;
			cs.second += it->getWhammy();
		}
	}
	// Set volume levels (averages of all instruments playing that track)
	for (auto const& track: m_song->music) {
		std::string const& name = track.first;
		std::string locName = _(name.c_str());  // FIXME: There should NOT be gettext calls here!
		double level = 1.0;
		if (volume.find(locName) != volume.end()) {
			CountSum cs = volume[locName];
			if (cs.first > 0) level = cs.second / cs.first;
			if (m_song->music.size() <= 1) level = std::max(0.333, level);
		}
		m_audio.streamFade(name, level);
		if (pitchbend.find(locName) != pitchbend.end()) {
			CountSum cs = pitchbend[locName];
			level = cs.second;
			m_audio.streamBend(name, level);
		}
	}
}

void ScreenSing::activateNextScreen()
{
	Game* gm = Game::getSingletonPtr();

	m_database.addSong(m_song);
	if (m_database.scores.empty() || !m_database.reachedHiscore(m_song)) {
		// if no highscore reached..
	    gm->activateScreen("Playlist");
	}

	// Score window visible -> Enter quits to Players Screen
	if(!config["game/karaoke_mode"].i() && !m_song->hasDance() &&!m_song->hasDrums() &&!m_song->hasGuitars()) {
		Screen* s = gm->getScreen("Players");
		ScreenPlayers* ss = dynamic_cast<ScreenPlayers*> (s);
		assert(ss);
		ss->setSong(m_song);
		gm->activateScreen("Players");
	} else {
		gm->activateScreen("Playlist");
	}
}

void ScreenSing::manageEvent(input::NavEvent const& event) {
	keyPressed = true;
	input::NavButton nav = event.button;
	m_quitTimer.setValue(QUIT_TIMEOUT);
	double time = m_audio.getPosition();
	Song::Status status = m_song->status(time);
	// When score window is displayed
	if (m_score_window.get()) {
		if (nav == input::NAV_START || nav == input::NAV_CANCEL) activateNextScreen();
		return;  // The rest are only available when score window is not displayed
	}
	// Instant quit with CANCEL at the very beginning
	if (nav == input::NAV_CANCEL && time < 1.0) {
		Game::getSingletonPtr()->activateScreen("Playlist");
		return;
	}

	if (event.repeat == 0 && devCanParticipate(event.devType)) {
		Game* gm = Game::getSingletonPtr();
		input::DevicePtr dev = gm->controllers.registerDevice(event.source);
		if (dev) {
			// Eat all events and see if any are valid for joining
			input::DevType type = input::DEVTYPE_GENERIC;
			std::string msg;
			for (input::Event ev; dev->getEvent(ev);) {
				if (ev.value == 0.0) continue;
				if (dev->type == input::DEVTYPE_DANCEPAD && m_song->hasDance()) {
					if (ev.button == input::DANCEPAD_UP) type = dev->type;
					else msg = dev->source.isKeyboard() ? _("Press UP to join dance!") : _("Step UP to join!");
				}
				else if (dev->type == input::DEVTYPE_GUITAR && m_song->hasGuitars()) {
					if (ev.button == input::GUITAR_GREEN) type = dev->type;
					else if (ev.button != input::GUITAR_WHAMMY && ev.button != input::GUITAR_GODMODE) {
						msg = dev->source.isKeyboard() ? _("Press 1 to join guitar!") : _("Press GREEN to join!");
					}
				}
				else if (dev->type == input::DEVTYPE_DRUMS && m_song->hasDrums()) {
					if (ev.button == input::DRUMS_KICK) type = dev->type;
					else msg = dev->source.isKeyboard() ? _("Press SPACE to join drums!") : _("KICK to join!");
				}
			}
			if (!msg.empty()) gm->flashMessage(msg, 0.0, 0.1, 0.1);
			else if (type == input::DEVTYPE_DANCEPAD) m_instruments.push_back(new DanceGraph(m_audio, *m_song, dev));
			else if (type != input::DEVTYPE_GENERIC) m_instruments.push_back(new GuitarGraph(m_audio, *m_song, dev, m_instruments.size()));
		}
	}

	// Only pause or esc opens the global menu (instruments have their own menus)
	// TODO: This should probably check if the source is participating as an instrument or not rather than check for its type
	if (!devCanParticipate(event.devType) && (nav == input::NAV_PAUSE || nav == input::NAV_CANCEL) && !m_audio.isPaused() && !m_menu.isOpen()) {
		m_menu.open();
		m_audio.togglePause();
	}
	// Global/singer pause menu navigation
	if (m_menu.isOpen()) {
		int do_action = 0;
		if (nav == input::NAV_START) { do_action = 1; }
		else if (nav == input::NAV_LEFT) { do_action = -1; }
		else if (nav == input::NAV_RIGHT) { do_action = 1; }
		else if (nav == input::NAV_DOWN) { m_menu.move(1); return; }
		else if (nav == input::NAV_UP) { m_menu.move(-1); return; }

		if (do_action != 0) {
			m_menu.action(do_action);
			// Did the action close the menu?
			if (!m_menu.isOpen() && m_audio.isPaused()) {
				m_audio.togglePause();
			}
			return;
		}
	}
	// Start button has special functions for skipping things (only in singing for now)
	if (nav == input::NAV_START && m_instruments.empty() && !m_layout_singer.empty() && !m_audio.isPaused()) {
		// Open score dialog early
		if (status == Song::FINISHED) {
			if (m_engine) m_engine->kill(); // Kill the engine thread
			m_score_window.reset(new ScoreWindow(m_instruments, m_database)); // Song finished, but no score window -> show it
		}
		// Skip instrumental breaks
		else if (status == Song::INSTRUMENTAL_BREAK) {
			if (time < 0) m_audio.seek(0.0);
			else {
				// TODO: Instead of calculating here, calculate instrumental breaks right after song loading and store in Song data structures
				double diff = getNaN();
				for (size_t i = 0; i < m_layout_singer.size(); ++i) {
					double d = m_layout_singer[i].lyrics_begin() - 3.0 - time;
					if (!(d > diff)) diff = d;  // Store smallest d in diff (notice NaN handling)
				}
				if (diff > 0.0) m_audio.seek(diff);
			}
		}
	}
}


void ScreenSing::manageEvent(SDL_Event event) {
	keyPressed = true;
	double time = m_audio.getPosition();
	SDL_Scancode key = event.key.keysym.scancode;
	// Ctrl combinations that can be used while performing (not when score dialog is displayed)
	if (event.type == SDL_KEYDOWN && (event.key.keysym.mod & Platform::shortcutModifier(false)) && !m_score_window.get()) {
		if (key == SDL_SCANCODE_C) {
			m_audio.toggleCenterChannelSuppressor();
			++config["audio/suppress_center_channel"];
			dispInFlash(config["audio/suppress_center_channel"]);
		}
		if (key == SDL_SCANCODE_S) m_audio.toggleSynth(m_song->getVocalTrack(m_selectedTrack).notes);
		if (key == SDL_SCANCODE_V) m_audio.streamFade("vocals", event.key.keysym.mod & KMOD_SHIFT ? 1.0 : 0.0);
		if (key == SDL_SCANCODE_K)  { // Toggle karaoke mode
			if(config["game/karaoke_mode"].i() >=2) config["game/karaoke_mode"].i() = 0;
			else ++config["game/karaoke_mode"];
			dispInFlash(config["game/karaoke_mode"]);
		}
		if (key == SDL_SCANCODE_H) {
			config["game/Textstyle"].i() ?  config["game/Textstyle"].i() = 0 : ++config["game/Textstyle"];
			dispInFlash(config["game/Textstyle"]);
			}
		if (key == SDL_SCANCODE_W) dispInFlash(++config["game/pitch"]); // Toggle pitch wave
		// Toggle webcam
		if (key == SDL_SCANCODE_A && Webcam::enabled()) {
			// Initialize if we haven't done that already
			if (!m_cam) { try { m_cam.reset(new Webcam(config["graphic/webcamid"].i())); } catch (...) { }; }
			if (m_cam) { dispInFlash(++config["graphic/webcam"]); m_cam->pause(!config["graphic/webcam"].b()); }
		}
		// Latency settings
		if (key == SDL_SCANCODE_COMMA) dispInFlash(--config["audio/video_delay"]);
		if (key == SDL_SCANCODE_PERIOD) dispInFlash(++config["audio/video_delay"]);
		if (key == SDL_SCANCODE_MINUS) dispInFlash(--config["audio/round-trip"]);
		if (key == SDL_SCANCODE_EQUALS) dispInFlash(++config["audio/round-trip"]);
		if (key == SDL_SCANCODE_LEFTBRACKET) dispInFlash(--config["audio/controller_delay"]);
		if (key == SDL_SCANCODE_RIGHTBRACKET) dispInFlash(++config["audio/controller_delay"]);
		bool seekback = false;

		if (m_song->danceTracks.empty()) { // Seeking backwards is currently not permitted for dance songs
			if (key == SDL_SCANCODE_HOME) { m_audio.seekPos(0.0); seekback = true; }
			if (key == SDL_SCANCODE_LEFT) {
				Song::SongSection section("error", 0);
				if (m_song->getPrevSection(m_audio.getPosition(), section)) {
					m_audio.seekPos(section.begin);
					// TODO: display popup with section.name here
					std::cout << section.name << std::endl;
				} else m_audio.seek(-5.0);
				seekback = true;
			}
		}
		if (key == SDL_SCANCODE_RIGHT) {
			Song::SongSection section("error", 0);
			if (m_song->getNextSection(m_audio.getPosition(), section)) {
				m_audio.seekPos(section.begin);
				// TODO: display popup with section.name here
				std::cout << section.name << std::endl;
			} else m_audio.seek(5.0);
		}

		// Some things must be reset after seeking backwards
		if (seekback)
			for (unsigned i = 0; i < m_layout_singer.size(); ++i)
				m_layout_singer[i].reset();
		// Reload current song
		if (key == SDL_SCANCODE_R) {
			exit(); m_song->reload(); enter();
			m_audio.seek(time);
		}
	}
}

namespace {
	const double arMin = 1.33;
	const double arMax = 2.35;

	void fillBG() {
		Dimensions dim(arMin);
		dim.fixedWidth(1.0);
		glutil::VertexArray va;
		va.texCoord(0,0).vertex(dim.x1(), dim.y1());
		va.texCoord(0,0).vertex(dim.x2(), dim.y1());
		va.texCoord(0,0).vertex(dim.x1(), dim.y2());
		va.texCoord(0,0).vertex(dim.x2(), dim.y2());
		getShader("texture").bind();
		va.draw();
	}
}

void ScreenSing::prepare() {
	Game* gm = Game::getSingletonPtr();
	// Enable/disable controllers as needed (mostly so that keyboard navigation will not be obstructed).
	gm->controllers.enableEvents(m_song->hasControllers() && !m_menu.isOpen() && !m_score_window.get());
	double time = m_audio.getPosition();
	if (m_video) m_video->prepare(time);
	// Menu mangling
	// We don't allow instrument menus during global menu
	// except for joining, in which case global menu is closed
	if (m_menu.isOpen()) {
		for (auto& i: m_instruments) {
			if (i.joining(time)) m_menu.close(); else i.toggleMenu(0);
		}
	}
}

/// Test if a given device type can join the current song.
// TODO: Somehow avoid duplicating these same checks in ScreenSing::prepare.
bool ScreenSing::devCanParticipate(input::DevType const& devType) const {
	if (devType == input::DEVTYPE_DANCEPAD && m_song->hasDance()) return true;
	if (devType == input::DEVTYPE_GUITAR && m_song->hasGuitars()) return true;
	if (devType == input::DEVTYPE_DRUMS && m_song->hasDrums()) return true;
	return false;	
}


void ScreenSing::draw() {
	// Get the time in the song
	double length = m_audio.getLength();
	double time = m_audio.getPosition();
	time -= config["audio/video_delay"].f();
	double songPercent = clamp(time / length);

	// Rendering starts
	{
		Transform ft(farTransform());
		double ar = arMax;
		// Background image
		if (!m_background || m_background->empty()) m_background.reset(new Surface(m_backgrounds.getRandom()));
		ar = m_background->dimensions.ar();
		if (ar > arMax || (m_video && ar > arMin)) fillBG();  // Fill white background to avoid black borders
		m_background->draw();
		// Webcam
		if (m_cam && config["graphic/webcam"].b()) m_cam->render();
		// Video
		if (m_video) {
			m_video->render(time); double tmp = m_video->dimensions().ar(); if (tmp > 0.0) ar = tmp;
		}
		// Top/bottom borders
		ar = clamp(ar, arMin, arMax);
		double offset = 0.5 / ar + 0.2;
		theme->bg_bottom.dimensions.fixedWidth(1.0).bottom(offset);
		theme->bg_bottom.draw();
		theme->bg_top.dimensions.fixedWidth(1.0).top(-offset);
		theme->bg_top.draw();
	}

	for (unsigned i = 0; i < m_layout_singer.size(); ++i) m_layout_singer[i].hideLyrics(m_audio.isPaused());

	instrumentLayout(time);

	bool fullSinger = m_instruments.empty() && m_layout_singer.size() <= 1;
	for (unsigned i = 0; i < m_layout_singer.size(); ++i) {
		m_layout_singer[i].draw(time, fullSinger ? LayoutSinger::FULL : (i == 0 ? LayoutSinger::TOP : LayoutSinger::BOTTOM));
	}

	Song::Status status = m_song->status(time);

	// Compute and draw the timer and the progressbar
	{
		unsigned t = clamp(time, 0.0, length);
		m_progress->dimensions.fixedWidth(0.4).left(-0.5).screenTop();
		theme->timer.dimensions.screenTop(0.5 * m_progress->dimensions.h());
		theme->songinfo.dimensions.screenBottom(-0.01);
		m_progress->draw(songPercent);

		Song::SongSection section("error", 0);
		std::string statustxt;
		if (m_song->getPrevSection(t - 1.0, section)) {
			statustxt = (boost::format("%02u:%02u - %s") % (t / 60) % (t % 60) % section.name).str();
		} else  statustxt = (boost::format("%02u:%02u") % (t / 60) % (t % 60)).str();

		if (!m_score_window.get() && m_instruments.empty() && !m_layout_singer.empty()) {
			if (status == Song::INSTRUMENTAL_BREAK)  statustxt += _("   ENTER to skip instrumental break");
			if (status == Song::FINISHED && !config["game/karaoke_mode"].i()) {
				if(config["game/autoplay"].b()) {
					if(m_displayAutoPlay) {
						statustxt += _("   Autoplay enabled");
					}
					else {
						 statustxt += _("   Remember to wait for grading!");
					}

					if(m_statusTextSwitch.get() == 0) {
					m_displayAutoPlay = !m_displayAutoPlay;
					m_statusTextSwitch.setValue(1);
					}
					} else {
					statustxt += _("   Remember to wait for grading!");
				}
			} else if(status == Song::FINISHED && config["game/autoplay"].b()) {
				statustxt += _("   Autoplay enabled");
			}
		}

		theme->timer.draw(statustxt);
	}

	if (config["game/karaoke_mode"].i() && !m_song->hasControllers()) { //guitar track? display the score window anyway!
		if (!m_audio.isPlaying()) {
			Game* gm = Game::getSingletonPtr();
			gm->activateScreen("Playlist");
			return;
		}
	} else {
		if (m_score_window.get()) {
			// Score window has been created (we are near the end)
			if (m_score_window->empty()) {  // No players to display scores for
				if (!m_audio.isPlaying()) { activateNextScreen(); return; }
			} else {  // Window being displayed
				if (m_quitTimer.get() == 0.0 && !m_audio.isPaused()) { activateNextScreen(); return; }
				m_score_window->draw();
			}
		}
		else if (!m_audio.isPlaying() || (status == Song::FINISHED
		  && m_audio.getLength() - time <= (m_song->instrumentTracks.empty() && m_song->danceTracks.empty() ? 3.0 : 0.2) )) {
			// Time to create the score window
			m_quitTimer.setValue(QUIT_TIMEOUT);
			if (m_engine) m_engine->kill(); // kill the engine thread (to avoid consuming memory)
			m_score_window.reset(new ScoreWindow(m_instruments, m_database));
		}
	}

	// Menus on top of everything
	for (auto& i: m_instruments) if (i.menuOpen()) i.drawMenu();
	if (m_menu.isOpen()) drawMenu();
	if(!keyPressed && m_DuetTimeout.get() == 0) {
		m_menu.action();
		}
	std::string songinfo = m_song->artist + " - " + m_song->title;
	theme->songinfo.draw(songinfo);
}

void ScreenSing::drawMenu() {
	if (m_menu.empty()) return;
	// Some helper vars
	ThemeInstrumentMenu& th = *m_menuTheme;
	const auto cur = &m_menu.current();
	double w = m_menu.dimensions.w();
	const float txth = th.option_selected.h();
	const float step = txth * 0.85f;
	const float h = m_menu.getOptions().size() * step + step;
	float y = -h * .5f + step;
	float x = -w * .5f + step;
	// Background
	th.bg.dimensions.middle(0).center(0).stretch(w, h);
	th.bg.draw();
	// Loop through menu items
	w = 0;
	int player = 0;
	boost::ptr_vector<Analyzer>& analyzers = m_audio.analyzers();
	for (MenuOptions::const_iterator it = m_menu.begin(); it != m_menu.end(); ++it) {
		// Pick the font object
		SvgTxtTheme* txt = &th.option_selected;
		if (cur != &*it)
			txt = &(th.getCachedOption(it->getName()));
		// Set dimensions and draw
		txt->dimensions.middle(x).center(y);
		txt->draw(it->getName());
		if (it->value == &m_vocalTracks[player]) {
					if(boost::lexical_cast<size_t>(player) < analyzers.size()) {
				Color color = MicrophoneColor::get(analyzers[player].getId());
				ColorTrans c(color);
				m_player_icon->dimensions.right(x).fixedHeight(0.040).center(y);
				m_player_icon->draw();
			}
			player++;
		}

		w = std::max(w, txt->w() + 2 * step); // Calculate the widest entry
		y += step;
	}
	if (cur->getComment() != "") {
		th.comment.dimensions.middle(0).screenBottom(-0.08);
		th.comment.draw(cur->getComment());
	}
	m_menu.dimensions.stretch(w, h);
}


ScoreWindow::ScoreWindow(Instruments& instruments, Database& database):
  m_database(database),
  m_pos(0.8, 2.0),
  m_bg(findFile("score_window.svg")),
  m_scoreBar(findFile("score_bar_bg.svg"), findFile("score_bar_fg.svg"), ProgressBar::VERTICAL, 0.0, 0.0, false),
  m_score_text(findFile("score_txt.svg")),
  m_score_rank(findFile("score_rank.svg"))
{
	Game::getSingletonPtr()->showLogo();
	m_pos.setTarget(0.0);
	m_database.scores.clear();
	// Singers
	for (auto p = m_database.cur.begin(); p != m_database.cur.end();) {
		ScoreItem item; item.type = input::DEVTYPE_VOCALS;
		item.score = p->getScore();
		if (item.score < 500) { p = m_database.cur.erase(p); continue; } // Dead
		item.track = "Vocals"; // For database
		item.track_simple = "vocals"; // For ScoreWindow
		item.color = Color(p->m_color.r, p->m_color.g, p->m_color.b);

		m_database.scores.push_back(item);
		++p;
	}
	// Instruments
	for (Instruments::iterator it = instruments.begin(); it != instruments.end();) {
		ScoreItem item;
		item.type = it->getGraphType();
		item.score = it->getScore();
		if (item.score < 100) { it = instruments.erase(it); continue; } // Dead
		item.track_simple = it->getTrack();
		item.track = it->getModeId();
		item.track[0] = toupper(item.track[0]); // Capitalize
		if (item.track_simple == TrackName::DRUMS) item.color = Color(0.1, 0.1, 0.1);
		else if (item.track_simple == TrackName::BASS) item.color = Color(0.5, 0.3, 0.1);
		else item.color = Color(1.0, 0.0, 0.0);

		m_database.scores.push_back(item);
		++it;
	}

	if (m_database.scores.empty())
		m_rank = _("No player!");
	else {
		// Determine winner
		m_database.scores.sort([](ScoreItem i, ScoreItem j) -> bool { return (i.score>j.score); });
		ScoreItem winner = *std::max_element(m_database.scores.begin(), m_database.scores.end());
		int topScore = winner.score;
		// Determine rank
		if (winner.type == input::DEVTYPE_VOCALS) {
			if (topScore > 8000) m_rank = _("Hit singer");
			else if (topScore > 6000) m_rank = _("Lead singer");
			else if (topScore > 4000) m_rank = _("Rising star");
			else if (topScore > 2000) m_rank = _("Amateur");
			else m_rank = _("Tone deaf");
		} else if (winner.type == input::DEVTYPE_DANCEPAD) {
			if (topScore > 8000) m_rank = _("Maniac");
			else if (topScore > 6000) m_rank = _("Hoofer");
			else if (topScore > 4000) m_rank = _("Rising star");
			else if (topScore > 2000) m_rank = _("Amateur");
			else m_rank = _("Loser");
		} else {
			if (topScore > 8000) m_rank = _("Virtuoso");
			else if (topScore > 6000) m_rank = _("Rocker");
			else if (topScore > 4000) m_rank = _("Rising star");
			else if (topScore > 2000) m_rank = _("Amateur");
			else m_rank = _("Tone deaf");
		}
	}
	m_bg.dimensions.middle().center();
}

void ScoreWindow::draw() {
	using namespace glmath;
	Transform trans(translate(vec3(0.0, m_pos.get(), 0.0)));
	m_bg.draw();
	const double spacing = 0.1 + 0.1 / m_database.scores.size();
	unsigned i = 0;

	for (Database::cur_scores_t::const_iterator p = m_database.scores.begin(); p != m_database.scores.end(); ++p, ++i) {
		int score = p->score;
		ColorTrans c(p->color);
		double x = spacing * (0.5 + i - 0.5 * m_database.scores.size());
		m_scoreBar.dimensions.fixedWidth(0.09).middle(x).bottom(0.20);
		m_scoreBar.draw(score / 10000.0);
		m_score_text.render(std::to_string(score));
		m_score_text.dimensions().middle(x).top(0.24).fixedHeight(0.042);
		m_score_text.draw();
		m_score_text.render(p->track_simple);
		m_score_text.dimensions().middle(x).top(0.20).fixedHeight(0.042);
		m_score_text.draw();
	}
	m_score_rank.draw(m_rank);
}

bool ScoreWindow::empty() {
	return m_database.scores.empty();
}
