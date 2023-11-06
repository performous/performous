#include "screen_songs.hh"

#include "audio.hh"
#include "configuration.hh"
#include "database.hh"
#include "hiscore.hh"
#include "i18n.hh"
#include "platform.hh"
#include "screen_sing.hh"
#include "screen_playlist.hh"
#include "songs.hh"
#include "util.hh"
#include "playlist.hh"
#include "graphic/video_driver.hh"
#include "theme/theme.hh"
#include "theme/theme_loader.hh"

#include "aubio/aubio.h"

#include <iostream>
#include <iomanip>
#include <mutex>
#include <sstream>

static const double IDLE_TIMEOUT = 35.0; // seconds

ScreenSongs::ScreenSongs(Game &game, std::string const& name, Audio& audio, Songs& songs, Database& database)
  : Screen(game, name), m_audio(audio), m_songs(songs), m_database(database) {
	m_songs.setAnimMargins(5.0, 5.0);
	// Using AnimValues as a simple timers counting seconds
	m_clock.setTarget(getInf());
	m_idleTimer.setTarget(getInf());
	game.getEventManager().addReceiver("onenter", std::bind(&ScreenSongs::onEnter, this, std::placeholders::_1));
}

void ScreenSongs::enter() {
	m_menu.close();
	m_songs.setFilter(m_search.text);
	m_audio.fadeout(getGame());
	m_menuPos = 1;
	m_infoPos = 0;
	m_jukebox = false;
	reloadGL();
}

void ScreenSongs::reloadGL() {
	auto loader = ThemeLoader();

	m_theme = loader.load<ThemeSongs>(getName());

	if (!m_theme)
		m_theme = std::make_unique<ThemeSongs>();

	m_menuTheme = std::make_unique<ThemeInstrumentMenu>();
	m_songbg_default = std::make_unique<Texture>(findFile("songs_bg_default.svg"));
	m_songbg_ground = std::make_unique<Texture>(findFile("songs_bg_ground.svg"));
	m_singCover = std::make_unique<Texture>(findFile("no_cover.svg"));
	m_instrumentCover = std::make_unique<Texture>(findFile("instrument_cover.svg"));
	m_bandCover = std::make_unique<Texture>(findFile("band_cover.svg"));
	m_danceCover = std::make_unique<Texture>(findFile("dance_cover.svg"));
	m_instrumentList = std::make_unique<Texture>(findFile("instruments.svg"));
}

void ScreenSongs::exit() {
	m_covers.clear();
	m_menu.clear();
	m_menuTheme.reset();
	m_singCover.reset();
	m_instrumentCover.reset();
	m_danceCover.reset();
	m_bandCover.reset();
	m_instrumentList.reset();
	m_theme.reset();
	m_video.reset();
	m_songbg.reset();
	m_songbg_default.reset();
	m_songbg_ground.reset();
	m_playing.clear();
}

/// Implement left/right on menu
void ScreenSongs::menuBrowse(Songs::SortChange dir) {
	switch (m_menuPos) {
		case 4: m_infoPos = (m_infoPos + to_underlying(dir) + 5) % 5; break;
		case 3: m_songs.typeChange(dir); break;
		case 2: m_songs.sortChange(getGame(), dir); break;
		case 1: m_songs.advance(to_underlying(dir)); break;
		case 0: /* no function on playlist yet */ break;
	}
}

void ScreenSongs::manageEvent(input::NavEvent const& event) {
	input::NavButton nav = event.button;
	// Handle basic navigational input that is possible also with instruments
	m_idleTimer.setValue(0.0);  // Reset idle timer
	if (nav == input::NavButton::PAUSE) m_audio.togglePause();
	else if (event.menu == input::NavMenu::A_PREV) {
		if (m_menu.isOpen()) m_menu.move(-1);
		else menuBrowse(Songs::SortChange::BACK);
	}
	else if (event.menu == input::NavMenu::A_NEXT) {
		if (m_menu.isOpen()) m_menu.move(1);
		else menuBrowse(Songs::SortChange::FORWARD);
	}
	else if (nav == input::NavButton::MOREUP) m_songs.advance(-10);
	else if (nav == input::NavButton::MOREDOWN) m_songs.advance(10);
	else if (m_jukebox) {
		if (nav == input::NavButton::CANCEL) m_jukebox = false;
		else if (nav == input::NavButton::START) { addSong(); sing(); }
		else if (event.menu == input::NavMenu::B_NEXT)  m_audio.seek(-5);
		else if (event.menu == input::NavMenu::B_PREV) m_audio.seek(5);
		else if (nav == input::NavButton::MOREUP) m_audio.seek(-30);
		else if (nav == input::NavButton::MOREDOWN) m_audio.seek(30);
	} else if (nav == input::NavButton::CANCEL) {
		if (m_menuPos != 1) m_menuPos = 1;  // Exit menu (back to song selection)
		else if (!m_search.text.empty()) { m_search.text.clear(); m_songs.setFilter(m_search.text); }  // Clear search
		else if (m_songs.typeNum()) m_songs.typeChange(Songs::SortChange::RESET);  // Clear type filter
		else getGame().activateScreen("Intro");
	}
	// The rest are only available when there are songs available
	else if (m_songs.empty()) return;
	else if (nav == input::NavButton::START) {
		if (m_menu.isOpen()) {
			m_menu.action(getGame());
		}
		else if (m_menuPos == 1 /* Cover browser */) {
			if (addSong()) sing();  // Add song and sing if it was the first to be added
		}
		else if (m_menuPos == 4) {
			m_menuPos = 1;
			m_jukebox = true;
		}
		else if (m_menuPos == 0 /* Playlist */) {
			if (getGame().getCurrentPlayList().isEmpty()) {
				m_menuPos = 1;
				addSong();
			} else {
				createPlaylistMenu();
				m_menu.open();
			}
		}
	}
	else if (event.menu == input::NavMenu::B_PREV) {
		if (m_menu.isOpen()) m_menu.move(-1);
		else if (m_menuPos < 4) ++m_menuPos;
	}
	else if (event.menu == input::NavMenu::B_NEXT) {
		if (m_menu.isOpen()) m_menu.move(1);
		else if (m_menuPos > 0) --m_menuPos;
	}
}

void ScreenSongs::manageEvent(SDL_Event event) {
	// Handle less common, keyboard only keys
	if (event.type == SDL_TEXTINPUT) {
		m_search += event.text.text;
		m_songs.setFilter(m_search.text);
	}
	else if (event.type == SDL_KEYDOWN) {
		SDL_Keysym keysym = event.key.keysym;
		int key = keysym.scancode;
		std::uint16_t mod = event.key.keysym.mod;
		if (key == SDL_SCANCODE_F4) m_jukebox = !m_jukebox;
		else if (key == SDL_SCANCODE_BACKSPACE) {
			m_search.backspace();
			m_songs.setFilter(m_search.text);
		}
		else if (!m_jukebox) {
			if (key == SDL_SCANCODE_R && mod & Platform::shortcutModifier()) {
				m_songs.reload();
				m_songs.setFilter(m_search.text);
				}
			// Shortcut keys for accessing different type filter modes.
			if (key == SDL_SCANCODE_TAB) m_songs.sortChange(getGame(), Songs::SortChange::FORWARD);
			if (key == SDL_SCANCODE_F5) m_songs.typeCycle(2);
			if (key == SDL_SCANCODE_F6) m_songs.typeCycle(3);
			if (key == SDL_SCANCODE_F7) m_songs.typeCycle(4);
			if (key == SDL_SCANCODE_F8) m_songs.typeCycle(1);
		}
	}
	if (m_songs.empty()) m_jukebox = false;
}

void ScreenSongs::update() {
	getGame().showLogo(!m_jukebox);
	if (m_idleTimer.get() < 0.3) return;  // Only update when the user gives us a break
	m_songs.update(); // Poll for new songs
	bool songChange = false;  // Do we need to switch songs?
	// Automatic song browsing
	if (!m_audio.isPaused() && m_idleTimer.get() > 1.0) {
		// If playback has ended or hasn't started
		if (!m_audio.isPlaying() || m_audio.getPosition() > m_audio.getLength()) {
			songChange = true;  // Force reload even if the music happens to stay the same
		}
		// If the above, or if in regular mode and idle too long, advance to next song
		if (songChange || (!m_jukebox && m_idleTimer.get() > IDLE_TIMEOUT)) {
			m_songs.advance(1);
			m_idleTimer.setValue(0.0);
		}
	}
	// Check out if the music has changed
	std::shared_ptr<Song> song = m_songs.currentPtr();
	Song::MusicFiles music;
	if (song) music = song->music;
	if (m_playing != music) songChange = true;
	// Switch songs if needed, only when the user is not browsing for a moment
	if (!songChange) return;
	ScreenSongs::previewBeatsBuffer.reset(new_fvec(1));
	{
	std::lock_guard<std::recursive_mutex> l(Audio::aubio_mutex);
	Audio::aubioTempo.reset(new_aubio_tempo("default", Audio::aubio_win_size, Audio::aubio_hop_size, static_cast<uint_t>(Audio::getSR())));
	}
	if (song && song->hasControllers()) { song->loadNotes(); } // Needed for BPM info.
	m_playing = music;
	// Clear the old content and load new content if available
	m_songbg.reset(); m_video.reset();
	double pstart = (!m_jukebox && song ? song->preview_start : 0.0);
	m_audio.playMusic(getGame(), music, true, 1.0, pstart);
	if (song) {
		fs::path const& background = song->background.empty() ? song->cover : song->background;
		if (!background.empty()) try { m_songbg = std::make_unique<Texture>(background); } catch (std::exception const&) {}
		if (!song->video.empty() && config["graphic/video"].b()) m_video = std::make_unique<Video>(song->video, song->videoGap);
	}
}

bool ScreenSongs::addSong() {
	auto& pl = getGame().getCurrentPlayList();
	bool empty = pl.getList().empty();
	pl.addSong(m_songs.currentPtr());
	return empty;
}

void ScreenSongs::sing() {
	ScreenSing& ss = dynamic_cast<ScreenSing&>(*getGame().getScreen("Sing"));
	ss.setSong(getGame().getCurrentPlayList().getNext());
	getGame().activateScreen("Sing");
}

void ScreenSongs::prepare() {
	double time = m_audio.getPosition() - config["audio/video_delay"].f();
	if (m_video) m_video->prepare(time);
}

void ScreenSongs::drawJukebox() {
	auto& window = getGame().getWindow();
	double pos = m_audio.getPosition();
	double len = m_audio.getLength();
	double diff = len - pos;
	if (pos < diff) diff = pos;  // Diff from beginning instead of from end
	if (!m_songbg.get() && !m_video.get()) diff = 0.0;  // Always display song name if there is no background
	if (diff < 3.0) {
		Song& song = m_songs.current();
		// Draw the cover
		Texture* cover = nullptr;
		if (!song.cover.empty()) cover = loadTextureFromMap(song.cover);
		if (cover && !cover->empty()) {
			Texture& s = *cover;
			s.dimensions.left(m_theme->song.dimensions.x1()).top(m_theme->song.dimensions.y2() + 0.05f).fitInside(0.15f, 0.15f);
			s.draw(window);
		}
		// Format && draw the song information text
		std::ostringstream oss_song;
		oss_song << song.title << '\n' << song.artist;
		m_theme->song.draw(window, oss_song.str());
	}
}

void ScreenSongs::drawMultimedia() {
	auto& window = getGame().getWindow();
	if (!m_songs.empty()) {
		Transform ft(window, farTransform());  // 3D effect
		double length = m_audio.getLength();
		double time = clamp(m_audio.getPosition() - config["audio/video_delay"].f(), 0.0, length);
		m_songbg_default->draw(window);   // Default bg
		if (m_songbg.get() && !m_video.get()) {
			if (m_songbg->width() > 512 && m_songbg->dimensions.ar() > 1.1f) {
				// Full screen mode
				float s = static_cast<float>(sin(m_clock.get()) * 0.15 + 1.15);
				Transform sc(window, glmath::scale(glmath::vec3(s, s, s)));
				m_songbg->draw(window);
			} else {
				// Low res texture or cover image, render in tiled mode
				double x = 0.05 * m_clock.get();
				m_songbg->draw(window, m_songbg->dimensions, TexCoords(static_cast<float>(x), 0.0f, static_cast<float>(x + 5.0), 5.0f));
			}
		}
		if (m_video.get()) m_video->render(window, time);
	}
	if (!m_jukebox) {
		m_songbg_ground->draw(window);
		m_theme->bg->draw(window);
		drawCovers();
	}
}

void ScreenSongs::draw() {
	update();
	drawMultimedia();

	std::ostringstream oss_song, oss_order;
	auto hiscore = std::string{};

	// Test if there are no songs
	if (m_songs.empty()) {
		// Format the song information text
		if (!m_search.text.empty()) {
			oss_song << _("Sorry, no songs match the search!");
			oss_order << m_search.text;
		} else if (m_songs.typeNum()) {
			oss_song << _("Sorry, no songs match the filter!");
			oss_order << m_songs.typeDesc();
		} else {
			oss_song << _("No songs found!");
			oss_order << _("Visit performous.org for free songs");
		}
	} else {
		Song& song = m_songs.current();
		// Format the song information text
		oss_song << song.artist << ": " << song.title;
		hiscore = getHighScoreText();
		// Escaped bytes of UTF-8 must be used here for compatibility with Windows (MSVC, mingw)
		char const* VERT_ARROW = "\xe2\x86\x95 ";  // ↕
		char const* HORIZ_ARROW = "\xe2\x86\x94 ";  // ↔
		char const* ENTER = "\xe2\x86\xb5 ";  // ↵
		char const* PAD = "   ";
		switch (m_menuPos) {
		case 1:
			if (!m_search.text.empty()) oss_order << m_search.text;
			else if (m_songs.typeNum()) oss_order << m_songs.typeDesc();
			else if (m_songs.sortNum()) oss_order << m_songs.getSortDescription();
			else oss_order << _("<type in to search>") << PAD << HORIZ_ARROW << _("songs") << PAD << VERT_ARROW << _("options");
			break;
		case 2: oss_order << HORIZ_ARROW << _("sort order: ") << m_songs.getSortDescription(); break;
		case 3: oss_order << HORIZ_ARROW << _("type filter: ") << m_songs.typeDesc(); break;
		case 4: oss_order << HORIZ_ARROW << _("hiscores") << PAD << ENTER << _("jukebox mode"); break;
		case 0:
			bool empty = getGame().getCurrentPlayList().isEmpty();
			oss_order << ENTER << (empty ? _("start a playlist with this song!") : _("open the playlist menu"));
			break;
		}
	}

	if (m_jukebox) drawJukebox();
	else {
		auto& window = getGame().getWindow();
		// Draw song and order texts
		m_theme->song.draw(window, oss_song.str());
		m_theme->order.draw(window, oss_order.str());
		drawInstruments(Dimensions(1.0f).fixedHeight(0.09f).right(0.45f).screenTop(0.02f));
		m_theme->hiscores.draw(window, hiscore);
	}

	drawImages(*m_theme);

	// Menus on top of everything
	if (m_menu.isOpen()) drawMenu();
}

std::string ScreenSongs::getHighScoreText() const {
	auto const scores = m_database.queryPerSongHiscore(m_songs.currentPtr());
	auto const datetimeFormat = config["game/datetime_format"].so();
	auto const maxLines = 8;

	// Reorder hiscores by track / score
	std::map<std::string, std::multiset<HiscoreItem>> scoresByTrack;
	for (auto const& hi: scores)
		scoresByTrack[hi.track].insert(hi);

	auto const scoreFormatter = [](auto& stream, auto const& score){
		stream << std::setw(10) << std::right << score << " \t";
	};
	auto const playerFormatter = [this](auto& stream, auto const& playerId) {
		const auto player = m_database.getPlayers().lookup(playerId);

		stream << std::setw(25) << std::left << player.value_or("Unknown player Id " + std::to_string(playerId));
	};
	auto const timeFormatter = [datetimeFormat](auto& stream, auto const& unixtime){
		if(unixtime.count()) {
			stream << " \t" << format(unixtime, datetimeFormat);
		}
	};
	auto stream = std::stringstream();
	auto n = 0;
	for (auto const& [track, scores]: scoresByTrack) {
		stream << track << ":\n";
		for (auto const& score: scores) {
			scoreFormatter(stream, score.score);
			playerFormatter(stream, score.playerid);
			timeFormatter(stream, score.unixtime);
			stream << "\n";
		}
		stream << "\n";
		if(++n == maxLines) {
			break;
		}
	}

	return stream.str();
}

void ScreenSongs::onEnter(EventParameter const& parameter) {
	if (parameter.get<std::string>("screen", "") != getName())
		return;

	auto const it = m_theme->events.find("onenter");

	if (it == m_theme->events.end())
		return;

	for (auto const& imageConfig : it->second.images) {
		auto image = findImage(imageConfig.id, *m_theme);

		if (image)
			imageConfig.update(*image);
	}
}

void ScreenSongs::drawCovers() {
	auto& window = getGame().getWindow();
	double spos = m_songs.currentPosition(); // This needs to be polled to run the animation
	int ss = static_cast<int>(m_songs.size());
	std::ptrdiff_t currentId = m_songs.currentId();
	double baseidx = spos + 1.5; --baseidx; // Round correctly
	double shift = spos - baseidx;
	// Calculate beat
	double beat = 0.5 + m_idleTimer.get() / 2.0;  // 30 BPM
	if (ss > 0) {
		// Use actual song BPM. FIXME: Should only do this if currentId is also playing.
		if (m_songs.currentPtr() && m_songs.currentPtr()->music == m_playing) {
				if (m_songs.currentPtr()->hasControllers() || !m_songs.currentPtr()->beats.empty()) {
				double t = m_audio.getPosition() - config["audio/video_delay"].f();
				Song::Beats const& beats = m_songs.current().beats;
				auto it = std::lower_bound(m_songs.currentPtr()->hasControllers() ? beats.begin() : (beats.begin() + 1), beats.end(), t);
				if (it != beats.begin() && it != beats.end()) {
					double t1 = *(it - 1), t2 = *it;
					beat = (t - t1) / (t2 - t1);
				}
			}
			else if (!m_songs.currentPtr()->m_bpms.empty()) {
				float tempo = static_cast<float>(m_songs.currentPtr()->m_bpms.front().step * 4.0);
				if (static_cast<unsigned>(tempo) <= 100u) tempo *= 2.0f;
				else if (static_cast<unsigned>(tempo) > 400u) tempo /= 4.0f;
				else if (static_cast<unsigned>(tempo) > 300u) tempo /= 3.0f;
				else if (static_cast<unsigned>(tempo) > 190u) tempo /= 2.0f;
				beat = 0.5 + m_idleTimer.get() / tempo;
			}
		}
	}
	beat = 1.0 + std::pow(std::abs(std::cos(0.5 * TAU * beat)), 10.0);  // Overdrive pulse
	// Draw covers and reflections
	int idx = static_cast<int>(baseidx);
	for (int i = -2; i < 6; ++i) {
		if (idx + i < 0 || idx + i >= ss) continue;
		Song& song = *m_songs[static_cast<unsigned>(idx + i)];
		Texture& s = getCover(song);
		// Calculate dimensions for cover and instrument markers
		float pos = static_cast<float>(static_cast<double>(i) - shift);
		// Function for highlight effect (offset = 0 for current cover), returns 0..1 highlight level
		auto highlightf = [=](float offset) { return smoothstep(3.5f, 0.0f, std::abs(pos + offset)); };
		// Coordinate translations (pos and offset in cover units to z and x in OpenGL space)
		auto ztrans = [=](float offset) { return -0.5f + 0.3f * highlightf(offset); };
		auto xtrans = [=](float offset) { return -0.2f + 0.20f * (pos + offset); };
		// A cover is angled to a line between the surrounding gaps (offset +- 0.5 covers)
		float angle = -std::atan2(ztrans(0.5f) - ztrans(-0.5f), xtrans(0.5f) - xtrans(-0.5f));
		float y = 0.5f * virtH();
		float x = xtrans(0.0f);
		float z = ztrans(0.0f);
		float c = 0.4f + 0.6f * highlightf(0.0f);
		if (m_menuPos == 1 /* Cover browser */ && idx + i == currentId) c = static_cast<float>(beat);
		using namespace glmath;
		Transform trans(window, translate(vec3(x, y, z)) * rotate(angle, vec3(0.0f, 1.0f, 0.0f)));
		ColorTrans c1(window, Color(c, c, c));
		s.dimensions.middle().screenCenter().bottom().fitInside(0.17f, 0.17f);
		// Draw the cover normally
		s.draw(window);
		// Draw the reflection
		Transform transMirror(window, scale(vec3(1.0f, -1.0f, 1.0f)));
		ColorTrans c2(window, Color::alpha(0.4f));
		s.draw(window);
	}
	// Draw the playlist
	auto const& playlist = getGame().getCurrentPlayList().getList();
	float c = static_cast<float>(m_menuPos == 0 /* Playlist */ ? beat : 1.0);
	ColorTrans c1(window, Color(c, c, c));
	for (size_t i = playlist.size() - 1; i < playlist.size(); --i) {
		Texture& s = getCover(*playlist[i]);
		float pos =  static_cast<float>(i) / std::max<float>(5.0f, static_cast<float>(playlist.size()));
		using namespace glmath;
		Transform trans(window,
		  translate(vec3(-0.35f + 0.06f * pos, 0.0f, 0.3f - 0.2f * pos))
		  * rotate(-0.0f, vec3(0.0f, 1.0f, 0.0f))
		);
		s.dimensions.middle().screenBottom(-0.06f).fitInside(0.08f, 0.08f);
		s.draw(window);
	}
}

Texture* ScreenSongs::loadTextureFromMap(fs::path path) {
	if(m_covers.find(path) == m_covers.end()) {
		m_covers.insert({ path, std::make_unique<Texture>(path) });
	}
	try {
		return m_covers.at(path).get();
	} catch (std::exception const&) {}
	return nullptr;
}

Texture& ScreenSongs::getCover(Song const& song) {
	Texture* cover = nullptr;
	// Fetch cover image from cache or try loading it
	if (!song.cover.empty()) cover = loadTextureFromMap(song.cover);
	// Fallback to background image as cover if needed
	if (!cover && !song.background.empty()) cover = loadTextureFromMap(song.background);
	// Use empty cover
	if (!cover) {
		if(song.hasDance()) {
			cover = m_danceCover.get();
		} else if(song.hasDrums()) {
			cover = m_bandCover.get();
		} else {
			size_t tracks = song.instrumentTracks.size();
			if (tracks == 0) cover = m_singCover.get();
			else cover = m_instrumentCover.get();
		}
	}
	return *cover;
}

namespace {
	float getIconTex(int i) {
		static int iconcount = 8;
		return static_cast<float>(i-1)/float(iconcount);
	}
	void drawIcon(int i, Dimensions const& dim) {
		glutil::VertexArray va;
		va.texCoord(getIconTex(i), 0.0f).vertex(dim.x1(), dim.y1());
		va.texCoord(getIconTex(i), 1.0f).vertex(dim.x1(), dim.y2());
		va.texCoord(getIconTex(i + 1), 0.0f).vertex(dim.x2(), dim.y1());
		va.texCoord(getIconTex(i + 1), 1.0f).vertex(dim.x2(), dim.y2());
		va.draw();
	}
}

void ScreenSongs::drawInstruments(Dimensions dim) const {
	bool have_bass = false;
	bool have_drums = false;
	bool have_dance = false;
	// TODO: Do something with is_karaoke
	//bool is_karaoke = false;
	int guitarCount = 0;
	int vocalCount = 0;
	if( !m_songs.empty() ) {
		Song const& song = m_songs.current();
		have_drums = song.hasDrums();
		have_dance = song.hasDance();
		//is_karaoke = (song.music.find("vocals") != song.music.end());
		if (song.hasVocals()) vocalCount = song.hasDuet() ? 2 : 1; // Make sure our generated duet track is not counted as a third vocal track for drawing.
		if (isTrackInside(song.instrumentTracks,TrackName::GUITAR)) guitarCount++;
		if (isTrackInside(song.instrumentTracks,TrackName::GUITAR_COOP)) guitarCount++;
		if (isTrackInside(song.instrumentTracks,TrackName::GUITAR_RHYTHM)) guitarCount++;
		if (isTrackInside(song.instrumentTracks,TrackName::BASS)) { guitarCount++; have_bass = true; }
	}

	TextureBinder tex(getGame().getWindow(), *m_instrumentList);
	float x = dim.x1();
	// dancing
	if (have_dance) {
		drawIcon(6, dim.left(x));
		x -= dim.w();
	}
	// drums
	if (have_drums) {
		drawIcon(4, dim.left(x));
		x -= dim.w();
	}
	// guitars & bass
	for (int i = guitarCount-1; i >= 0; i--) {
		drawIcon(i == 0 && have_bass ? 3 : 2, dim.left(x));
		x -= (i > 0 ? 0.3f : 1.0f) * dim.w();
	}
	for (int i = vocalCount-1; i >= 0; i--) {
		drawIcon(1, dim.left(x));
		x -= (i > 0 ? 0.3f : 1.0f) * dim.w();
	}
}

void ScreenSongs::drawMenu() {
	if (m_menu.empty()) return;
	auto& window = getGame().getWindow();
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
	th.bg->dimensions.middle(0).center(0).stretch(w, h);
	th.bg->draw(window);
	// Loop through menu items
	w = 0;
	for (MenuOptions::const_iterator it = m_menu.begin(); it != m_menu.end(); ++it) {
		// Pick the font object
		SvgTxtTheme* txt = &th.option_selected;
		if (cur != &*it)
			txt = &(th.getCachedOption(it->getName()));
		// Set dimensions and draw
		txt->dimensions.middle(x).center(y);
		txt->draw(window, it->getName());
		w = std::max(w, txt->w() + 2 * step); // Calculate the widest entry
		y += step;
	}
	if (cur->getComment() != "") {
		th.comment.dimensions.middle(0).screenBottom(-0.12f);
		th.comment.draw(window, cur->getComment());
	}
	m_menu.dimensions.stretch(w, h);
}

std::unique_ptr<fvec_t, void(*)(fvec_t*)> ScreenSongs::previewBeatsBuffer = std::unique_ptr<fvec_t, void(*)(fvec_t*)>(new_fvec(1), [](fvec_t* p){del_fvec(p);});

void ScreenSongs::createPlaylistMenu() {
	m_menu.clear();
	m_menu.add(MenuOption(_("Play"), "")).call([this]() {
		getGame().getCurrentPlayList().addSong(m_songs.currentPtr());
		m_menuPos = 1;
		m_menu.close();
		sing();
	});
	m_menu.add(MenuOption(_("Shuffle"), "")).call([this]() {
		getGame().getCurrentPlayList().shuffle();
		m_menuPos = 1;
		m_menu.close();
	});
	m_menu.add(MenuOption(_("View playlist"), "")).screen("Playlist");
	m_menu.add(MenuOption(_("Clear playlist"), "")).call([this]() {
		getGame().getCurrentPlayList().clear();
		m_menuPos = 1;
		m_menu.close();
	});
	m_menu.add(MenuOption(_("Back"), "")).call([this]() {
		m_menuPos = 1;
		m_menu.close();
	});
}
