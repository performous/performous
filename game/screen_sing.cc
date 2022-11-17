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
#include "analyzer.hh"
#include "platform.hh"
#include "screen_players.hh"
#include "songparser.hh"
#include "util.hh"
#include "video.hh"
#include "webcam.hh"
#include "screen_songs.hh"
#include "notegraphscalerfactory.hh"

#include <fmt/format.h>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <utility>

namespace {
	/// Add a flash message about the state of a config item
	void dispInFlash(ConfigItem& ci) {
		Game* gm = Game::getSingletonPtr();
		gm->flashMessage(ci.getShortDesc() + ": " + ci.getValue());
	}
}

ScreenSing::ScreenSing(std::string const& name, Audio& audio, Database& database, Backgrounds& bgs):
	Screen(name), m_audio(audio), m_database(database), m_backgrounds(bgs),
	m_selectedTrack(TrackName::VOCAL_LEAD)
{}

void ScreenSing::enter() {
	keyPressed = false;
	m_DuetTimeout.setValue(10);
	Game* gm = Game::getSingletonPtr();
	// Initialize webcam
	gm->loading(_("Initializing webcam..."), 0.1f);
	if (config["graphic/webcam"].b() && Webcam::enabled()) {
		try {
			m_cam = std::make_unique<Webcam>(config["graphic/webcamid"].ui());
		} catch (std::exception& e) { std::cout << e.what() << std::endl; };
	}
	// Load video
	gm->loading(_("Loading video..."), 0.2f);
	if (!m_song->video.empty() && config["graphic/video"].b()) {
		m_video = std::make_unique<Video>(m_song->video, m_song->videoGap);
	}
	reloadGL();
	// Load song notes
	gm->loading(_("Loading song..."), 0.4f);
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
	gm->loading(_("Loading menu..."), 0.7f);
	{
		m_duet = ConfigItem(static_cast<unsigned short>(0));
		for (size_t player = 0; player < players(); ++player) {
			ConfigItem& vocalTrack = m_vocalTracks[player];
			vocalTrack = ConfigItem(static_cast<unsigned short>(0));
		}
		prepareVoicesMenu();
	}
	gm->showLogo(false);
	gm->loading(_("Loading complete"), 1.0f);
}

void ScreenSing::prepareVoicesMenu(unsigned moveSelectionTo) {
		VocalTracks const& tracks = m_song->vocalTracks;
		m_menu.clear();
		m_menu.add(MenuOption(_("Start"), _("Start performing"))).call([this]{ setupVocals(); });

		if (players() > 1) { // Duet toggle
			m_duet.addEnum(_("Duet mode"));
			m_duet.addEnum(_("Normal mode"));
			m_menu.add(MenuOption("", _("Switch between duet and regular singing mode"))).changer(m_duet,"song/duet");
		}
		// Add vocal track selector for each player
		for (size_t player = 0; player < players(); ++player) {
			ConfigItem& vocalTrack = m_vocalTracks[player];
			for (auto const& track: tracks) vocalTrack.addEnum(track.second.name);
			if (tracks.size() > 1) {
				if (player % 2) vocalTrack.selectEnum(m_song->getVocalTrack(SongParserUtil::DUET_P2).name);  // Every other player gets the second track
				else vocalTrack.selectEnum(m_song->getVocalTrack(TrackName::VOCAL_LEAD).name);
			}
			m_menu.add(MenuOption("", _("Change vocal track"))).changer(vocalTrack);
			if (m_duet.ui() == 1) {
				vocalTrack.selectEnum(m_song->getVocalTrack(SongParserUtil::DUET_BOTH).name);
				break; // If duet mode is disabled, the vocal track selection for players beyond the first is ignored anyway.
			}
		}
		m_menu.add(MenuOption(_("Quit"), _("Exit to song browser"))).screen("Songs");
		m_menu.select(moveSelectionTo);
		m_menu.open();
		if (tracks.size() <= 1) setupVocals();  // No duet menu
}

void ScreenSing::setupVocals() {
	if (!m_song->vocalTracks.empty()) {
		m_layout_singer.clear();
		Engine::VocalTrackPtrs selectedTracks;
		auto& analyzers = m_audio.analyzers();
		//size_t players = (analyzers.empty() ? 1 : analyzers.size());  // Always at least 1; should be number of mics
		std::set<VocalTrack*> shownTracks;  // Tracks to be included in layout_singer (stored by name for proper sorting and merging duplicates)
		for (size_t player = 0; player < players(); ++player) {
			VocalTrack* vocal = &m_song->getVocalTrack(m_vocalTracks[(m_duet.ui() == 0u ? player : 0u)].ui());
			selectedTracks.push_back(vocal);
			shownTracks.insert(vocal);
		}

		//if (shownTracks.size() > 2) throw std::runtime_error("Too many tracks chosen. Only two vocal tracks can be used simultaneously.")
		for (auto const& trk: shownTracks) {
			const auto scaler = NoteGraphScalerFactory(config).create(*trk);
			auto layoutSingerPtr = std::unique_ptr<LayoutSinger>(std::make_unique<LayoutSinger>(*trk, m_database, scaler, theme));
			m_layout_singer.push_back(std::move(layoutSingerPtr));
		}
		// Note: Engine maps tracks with analyzers 1:1. If user doesn't have mics, we still want to have singer layout enabled but without engine...
		if (!analyzers.empty()) m_engine = std::make_unique<Engine>(m_audio, selectedTracks, m_database);
	}
	createPauseMenu();
	bool sameVoice = true;
	for (size_t player = 0; player < players(); ++player) {
		ConfigItem& vocalTrack = m_vocalTracks[player];
		if (player == 0) { m_selectedVocal = vocalTrack.ui(); }
		if (vocalTrack.ui() != m_selectedVocal) { sameVoice = false; break; }
	}
	m_singingDuet = (m_song->hasDuet() && m_duet.ui() == 0 && players() > 1 && sameVoice != true);
	m_audio.pause(false);
}

void ScreenSing::createPauseMenu() {
	m_menu.clear();
	m_menu.add(MenuOption(_("Resume"), _("Back to performing!")));
	m_menu.add(MenuOption(_("Restart"), _("Start the song\nfrom the beginning"))).screen("Sing");
	Game* gm = Game::getSingletonPtr();
	if(!gm->getCurrentPlayList().isEmpty() || config["game/autoplay"].b()){
		m_menu.add(MenuOption(_("Skip"), _("Skip current song"))).screen("Playlist");
	}
	m_menu.add(MenuOption(_("Quit"), _("Exit to song browser"))).call([]() {
		Game* gm = Game::getSingletonPtr();
		gm->activateScreen("Songs");
	});
	m_menu.close();
}

void ScreenSing::reloadGL() {
	// Load UI graphics
	theme = std::make_shared<ThemeSing>();
	m_menuTheme = std::make_unique<ThemeInstrumentMenu>();
	m_pause_icon = std::make_unique<Texture>(findFile("sing_pause.svg"));
	m_player_icon = std::make_unique<Texture>(findFile("sing_pbox.svg")); // For duet menu
	m_help = std::make_unique<Texture>(findFile("instrumenthelp.svg"));
	m_progress = std::make_unique<ProgressBar>(findFile("sing_progressbg.svg"), findFile("sing_progressfg.svg"), ProgressBar::Mode::HORIZONTAL, 0.01f, 0.01f, true);
	// Load background
	if (!m_song->background.empty()) m_background = std::make_unique<Texture>(m_song->background);
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
		if ((*it)->dead()) {
			it = m_instruments.erase(it);
			continue;
		}
		++count_alive;
		if ((*it)->menuOpen()) ++count_menu;
		++it;
	}
	if (count_alive > 0) {
		// Handle pause
		bool shouldPause = count_menu > 0 || m_menu.isOpen();
		if (shouldPause != m_audio.isPaused()) m_audio.togglePause();
	} else if (time < -0.5) {
		// Display help if no-one has joined yet
		ColorTrans c(Color::alpha(static_cast<float>(clamp(-1.0 - 2.0 * time))));
		m_help->draw();
	}
	double iw = std::min(0.5, 1.0 / count_alive);
	typedef std::pair<unsigned, double> CountSum;
	std::map<std::string, CountSum> volume; // Stream id to (count, sum)
	std::map<std::string, CountSum> pitchbend; // Stream id to (count, sum)
	for (Instruments::iterator it = m_instruments.begin(); it != m_instruments.end(); ++it, ++i) {
		(*it)->engine();
		(*it)->position(static_cast<float>((0.5f + static_cast<float>(i) - 0.5f * static_cast<float>(count_alive)) * iw), static_cast<float>(iw)); // Do layout stuff
		(*it)->draw(time);
		{
			CountSum& cs = volume[(*it)->getTrack()];
			cs.first++;
			cs.second += (*it)->correctness();
		}{
			CountSum& cs = pitchbend[(*it)->getTrack()];
			cs.first++;
			cs.second += (*it)->getWhammy();
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
		if (name == "Vocals") {
			m_audio.streamFade(name, config["audio/mute_vocals_track"].b() ? 0.0 : 1.0);
		}
		else {
			m_audio.streamFade(name, level);
		}
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
	if(!config["game/karaoke_mode"].ui() && !m_song->hasDance() &&!m_song->hasDrums() &&!m_song->hasGuitars()) {
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
	m_quitTimer.setValue(config["game/results_timeout"].ui());
	double time = m_audio.getPosition();
	Song::Status status = m_song->status(time, this);
	// When score window is displayed
	if (m_score_window.get()) {
		if (nav == input::NavButton::START || nav == input::NavButton::CANCEL) activateNextScreen();
		return;  // The rest are only available when score window is not displayed
	}
	// Instant quit with CANCEL at the very beginning
	if (nav == input::NavButton::CANCEL && time < 1.0) {
		if (m_menu.isOpen()) { m_menu.moveToLast(); }
		else { Game::getSingletonPtr()->activateScreen(config["game/autoplay"].b() ? "Songs" : "Playlist"); }
		return;
	}

	if (event.repeat == 0 && devCanParticipate(event.devType)) {
		Game* gm = Game::getSingletonPtr();
		input::DevicePtr dev = gm->controllers.registerDevice(event.source);
		if (dev) {
			// Eat all events and see if any are valid for joining
			input::DevType type = input::DevType::GENERIC;
			std::string msg;
			for (input::Event ev; dev->getEvent(ev);) {
				if (ev.value == 0.0) continue;
				if (dev->type == input::DevType::DANCEPAD && m_song->hasDance()) {
					if (ev.button == input::ButtonId::DANCEPAD_UP) type = dev->type;
					else msg = dev->source.isKeyboard() ? _("Press UP to join dance!") : _("Step UP to join!");
				}
				else if (dev->type == input::DevType::GUITAR && m_song->hasGuitars()) {
					if (ev.button == input::ButtonId::GUITAR_GREEN) type = dev->type;
					else if (ev.button != input::ButtonId::GUITAR_WHAMMY && ev.button != input::ButtonId::GUITAR_GODMODE) {
						msg = dev->source.isKeyboard() ? _("Press 1 to join guitar!") : _("Press GREEN to join!");
					}
				}
				else if (dev->type == input::DevType::DRUMS && m_song->hasDrums()) {
					if (ev.button == input::ButtonId::DRUMS_KICK) type = dev->type;
					else msg = dev->source.isKeyboard() ? _("Press SPACE to join drums!") : _("KICK to join!");
				}
			}
			if (!msg.empty()) gm->flashMessage(msg, 0.0f, 0.1f, 0.1f);
			else if (type == input::DevType::DANCEPAD) m_instruments.push_back(std::make_unique<DanceGraph>(m_audio, *m_song, dev));
			else if (type != input::DevType::GENERIC) m_instruments.push_back(std::make_unique<GuitarGraph>(m_audio, *m_song, dev, m_instruments.size()));
		}
	}

	// Only pause or esc opens the global menu (instruments have their own menus)
	// TODO: This should probably check if the source is participating as an instrument or not rather than check for its type
	if (!devCanParticipate(event.devType) && (nav == input::NavButton::PAUSE || nav == input::NavButton::CANCEL) && !m_audio.isPaused() && !m_menu.isOpen()) {
		m_menu.open();
		m_audio.togglePause();
	}
	// Global/singer pause menu navigation
	if (m_menu.isOpen()) {
		int do_action = 0;
		if (nav == input::NavButton::START) { do_action = 1; }
		else if (nav == input::NavButton::LEFT) {
			if (m_menu.current().type == MenuOption::Type::CHANGE_VALUE) { do_action = -1; }
			else { m_menu.move(-1); return; }
		}
		else if (nav == input::NavButton::RIGHT) {
			if (m_menu.current().type == MenuOption::Type::CHANGE_VALUE) { do_action = 1; }
			else { m_menu.move(1); return; }
			}
		else if (nav == input::NavButton::DOWN) { m_menu.move(1); return; }
		else if (nav == input::NavButton::UP) { m_menu.move(-1); return; }

		if (do_action != 0) {
			std::string currentOption = m_menu.current().getVirtName();
			m_menu.action(do_action);
			if (currentOption == "song/duet") { prepareVoicesMenu(m_menu.curIndex()); }
			// Did the action close the menu?
			if (!m_menu.isOpen() && m_audio.isPaused()) {
				m_audio.togglePause();
			}
			return;
		}
	}
	// Start button has special functions for skipping things (only in singing for now)
	if (nav == input::NavButton::START && m_instruments.empty() && !m_layout_singer.empty() && !m_audio.isPaused()) {
		// Open score dialog early
		if (status == Song::Status::FINISHED) {
			if (m_engine) m_engine->kill(); // Kill the engine thread
			m_score_window = std::make_unique<ScoreWindow>(m_instruments, m_database); // Song finished, but no score window -> show it
		}
		// Skip instrumental breaks
		else if (status == Song::Status::INSTRUMENTAL_BREAK) {
			if (time < 0) m_audio.seek(0.0);
			else {
				// TODO: Instead of calculating here, calculate instrumental breaks right after song loading and store in Song data structures
				double diff = getNaN();
				for (size_t i = 0; i < m_layout_singer.size(); ++i) {
					double d = m_layout_singer[i]->lyrics_begin() - 3.0 - time;
					if (!(d > diff)) diff = d;  // Store smallest d in diff (notice NaN handling)
				}
				if (diff > 0.0) m_audio.seek(diff);
			}
		}
	}
}


void ScreenSing::manageEvent(SDL_Event event) {
	keyPressed = true;
	// Check to see if a menu is open and bail out before changes can be made
	if (m_score_window.get() || m_menu.isOpen()) return;
	for (auto& i: m_instruments) if (!i->menuOpen()) return;
	double time = m_audio.getPosition();
	SDL_Scancode key = event.key.keysym.scancode;
	// Ctrl combinations that can be used while performing
	if (event.type == SDL_KEYDOWN && (event.key.keysym.mod & Platform::shortcutModifier())) {
		if (key == SDL_SCANCODE_C) {
			m_audio.toggleCenterChannelSuppressor();
			++config["audio/suppress_center_channel"];
			dispInFlash(config["audio/suppress_center_channel"]);
		}
		if (key == SDL_SCANCODE_S) m_audio.toggleSynth(m_song->getVocalTrack(m_selectedTrack).notes);
		if (key == SDL_SCANCODE_V) {
			config["audio/mute_vocals_track"].b() = !config["audio/mute_vocals_track"].b();
			m_audio.streamFade("Vocals", config["audio/mute_vocals_track"].b() ? 0.0 : 1.0);
			dispInFlash(config["audio/mute_vocals_track"]);
		}
		if (key == SDL_SCANCODE_K)  { // Toggle karaoke mode
			if(config["game/karaoke_mode"].ui() >=2) config["game/karaoke_mode"].ui() = 0;
			else ++config["game/karaoke_mode"];
			dispInFlash(config["game/karaoke_mode"]);
		}
		if (key == SDL_SCANCODE_H) {
			config["game/Textstyle"].ui() ? config["game/Textstyle"].ui() = 0 : ++config["game/Textstyle"].ui();
			dispInFlash(config["game/Textstyle"]);
			}
		if (key == SDL_SCANCODE_W) dispInFlash(++config["game/pitch"]); // Toggle pitch wave
		// Toggle webcam
		if (key == SDL_SCANCODE_A && Webcam::enabled()) {
			// Initialize if we haven't done that already
			if (!m_cam) { try { m_cam = std::make_unique<Webcam>(config["graphic/webcamid"].ui()); } catch (...) { }; }
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
				m_layout_singer[i]->reset();
		// Reload current song
		if (key == SDL_SCANCODE_R) {
			exit(); m_song->reload(); enter();
			m_audio.seek(time);
		}
	}
}

namespace {
	const float arMin = 1.33f;
	const float arMax = 2.35f;

	void fillBG() {
		Dimensions dim(arMin);
		dim.fixedWidth(1.0f);
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
			if (i->joining(time)) m_menu.close(); else i->toggleMenu(0);
		}
	}
}

/// Test if a given device type can join the current song.
// TODO: Somehow avoid duplicating these same checks in ScreenSing::prepare.
bool ScreenSing::devCanParticipate(input::DevType const& devType) const {
	if (devType == input::DevType::DANCEPAD && m_song->hasDance()) return true;
	if (devType == input::DevType::GUITAR && m_song->hasGuitars()) return true;
	if (devType == input::DevType::DRUMS && m_song->hasDrums()) return true;
	return false;
}

size_t ScreenSing::players() const {
	auto& analyzers = m_audio.analyzers();

	return (analyzers.empty() ? 1 : analyzers.size());
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
		float ar = arMax;
		// Background image
		if (!m_background || m_background->empty()) m_background = std::make_unique<Texture>(m_backgrounds.getRandom());
		ar = m_background->dimensions.ar();
		if (ar > arMax || (m_video && ar > arMin)) fillBG();  // Fill white background to avoid black borders
		m_background->draw();
		// Webcam
		if (m_cam && config["graphic/webcam"].b()) m_cam->render();
		// Video
		if (m_video) {
			m_video->render(time); float tmp = m_video->dimensions().ar(); if (tmp > 0.0f) ar = tmp;
		}
		// Top/bottom borders
		ar = clamp(ar, arMin, arMax);
		float offset = 0.5f / ar + 0.2f;
		theme->bg_bottom.dimensions.fixedWidth(1.0f).bottom(offset);
		theme->bg_bottom.draw();
		theme->bg_top.dimensions.fixedWidth(1.0f).top(-offset);
		theme->bg_top.draw();
	}

	for (unsigned i = 0; i < m_layout_singer.size(); ++i) m_layout_singer[i]->hideLyrics(m_audio.isPaused());

	instrumentLayout(time);

	bool fullSinger = m_instruments.empty() && m_layout_singer.size() <= 1;
	for (unsigned i = 0; i < m_layout_singer.size(); ++i) {
		m_layout_singer[i]->draw(time, fullSinger ? LayoutSinger::PositionMode::FULL : (i == 0 ? LayoutSinger::PositionMode::TOP : LayoutSinger::PositionMode::BOTTOM));
	}

	Song::Status status = m_song->status(time, this);

	// Compute and draw the timer and the progressbar
	{
		unsigned t = static_cast<unsigned>(clamp(time, 0.0, length));
		m_progress->dimensions.fixedWidth(0.4f).left(-0.5f).screenTop();
		theme->timer.dimensions.screenTop(0.5f * m_progress->dimensions.h());
		theme->songinfo.dimensions.screenBottom(-0.01f);
		m_progress->draw(static_cast<float>(songPercent));

		Song::SongSection section("error", 0);
		std::string statustxt;
		if (m_song->getPrevSection(t - 1.0, section)) {
			statustxt = fmt::format("{:02d}:{:02d} - {}", (t / 60), (t % 60), (section.name));
		} else {
			statustxt = fmt::format("{:02d}:{:02d}", (t / 60), (t % 60));
		}

		if (!m_score_window.get() && m_instruments.empty() && !m_layout_singer.empty()) {
			if (status == Song::Status::INSTRUMENTAL_BREAK) {
				statustxt += _("   ENTER to skip instrumental break");
			}
			if (status == Song::Status::FINISHED && !config["game/karaoke_mode"].ui()) {
				if(config["game/autoplay"].b()) {
					if(m_displayAutoPlay) {
						statustxt += _("   Autoplay enabled");
					} else {
						if(!m_audio.analyzers().empty()) {
							statustxt += _("   Remember to wait for grading!");
						} else {
							statustxt += _("   Prepare for the next song!");
						}
					}

					if(m_statusTextSwitch.get() == 0) {
						m_displayAutoPlay = !m_displayAutoPlay;
						m_statusTextSwitch.setValue(1);
					}
				} else {
					if(!m_audio.analyzers().empty()) {
						statustxt += _("   Remember to wait for grading!");
					} else {
						statustxt += _("   Choose your next song!");
					}
				}
			} else if(status == Song::Status::FINISHED && config["game/autoplay"].b()) {
				statustxt += _("   Autoplay enabled");
			}
		}

		theme->timer.draw(statustxt);
	}

	if (config["game/karaoke_mode"].ui() && !m_song->hasControllers()) { //guitar track? display the score window anyway!
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
		else if (!m_audio.isPlaying() || (status == Song::Status::FINISHED
		  && m_audio.getLength() - time <= (m_song->instrumentTracks.empty() && m_song->danceTracks.empty() ? 3.0 : 0.2) )) {
			// Time to create the score window
			m_quitTimer.setValue(config["game/results_timeout"].ui());
			if (m_engine) m_engine->kill(); // kill the engine thread (to avoid consuming memory)
			m_score_window = std::make_unique<ScoreWindow>(m_instruments, m_database);
		}
	}

	// Menus on top of everything
	for (auto& i: m_instruments) if (i->menuOpen()) i->drawMenu();
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
	float w = m_menu.dimensions.w();
	const float txth = th.option_selected.h();
	const float step = txth * 0.85f;
	const float h = static_cast<float>(m_menu.getOptions().size()) * step + step;
	float y = -h * .5f + step;
	float x = -w * .5f + step;
	// Background
	th.bg.dimensions.middle(0).center(0).stretch(w, h);
	th.bg.draw();
	// Loop through menu items
	w = 0;
	std::size_t player = 0;
	auto& analyzers = m_audio.analyzers();
	for (MenuOptions::const_iterator it = m_menu.begin(); it != m_menu.end(); ++it) {
		// Pick the font object
		SvgTxtTheme* txt = &th.option_selected;
		if (cur != &*it)
			txt = &(th.getCachedOption(it->getName()));
		// Set dimensions and draw
		txt->dimensions.middle(x).center(y);
		txt->draw(it->getName());
		if (it->value == &m_vocalTracks[player]) {
			if (player < analyzers.size()) {
				Color color = MicrophoneColor::get(analyzers[player].getId());
				ColorTrans c(color);
				m_player_icon->dimensions.right(x).fixedHeight(0.040f).center(y);
				m_player_icon->draw();
			}
			player++;
		}

		w = std::max(w, txt->w() + 2 * step); // Calculate the widest entry
		y += step;
	}
	if (cur->getComment() != "") {
		th.comment.dimensions.middle(0.0f).screenBottom(-0.08f);
		th.comment.draw(cur->getComment());
	}
	m_menu.dimensions.stretch(w, h);
}

