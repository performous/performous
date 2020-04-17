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
#include "theme.hh"
#include "util.hh"
#include "playlist.hh"

#include "aubio/aubio.h"
#include <iostream>
#include <mutex>
#include <sstream>

static const double IDLE_TIMEOUT = 35.0; // seconds

ScreenSongs::ScreenSongs(std::string const& name, Audio& audio, Songs& songs, Database& database):
  Screen(name), m_audio(audio), m_songs(songs), m_database(database)
{
	m_songs.setAnimMargins(5.0, 5.0);
	// Using AnimValues as a simple timers counting seconds
	m_clock.setTarget(getInf());
	m_idleTimer.setTarget(getInf());
}

void ScreenSongs::enter() {
	m_menu.close();
	m_songs.setFilter(m_search.text);
	m_audio.fadeout();
	m_menuPos = 1;
	m_infoPos = 0;
	m_jukebox = false;
	reloadGL();
}

void ScreenSongs::reloadGL() {
	theme = std::make_unique<ThemeSongs>();
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
	theme.reset();
	m_video.reset();
	m_songbg.reset();
	m_songbg_default.reset();
	m_songbg_ground.reset();
	m_playing.clear();
}

/// Implement left/right on menu
void ScreenSongs::menuBrowse(int dir) {
	switch (m_menuPos) {
		case 4: m_infoPos = (m_infoPos + dir + 5) % 5; break;
		case 3: m_songs.typeChange(dir); break;
		case 2: m_songs.sortChange(dir); break;
		case 1: m_songs.advance(dir); break;
		case 0: /* no function on playlist yet */ break;
	}
}

void ScreenSongs::manageEvent(input::NavEvent const& event) {
	Game* gm = Game::getSingletonPtr();
	input::NavButton nav = event.button;
	// Handle basic navigational input that is possible also with instruments
	m_idleTimer.setValue(0.0);  // Reset idle timer
	if (nav == input::NAV_PAUSE) m_audio.togglePause();
	else if (event.menu == input::NAVMENU_A_PREV) {
		if (m_menu.isOpen()) m_menu.move(-1);
		else menuBrowse(-1);
	}
	else if (event.menu == input::NAVMENU_A_NEXT) {
		if (m_menu.isOpen()) m_menu.move(1);
		else menuBrowse(1);
	}
	else if (nav == input::NAV_MOREUP) m_songs.advance(-10);
	else if (nav == input::NAV_MOREDOWN) m_songs.advance(10);
	else if (m_jukebox) {
		if (nav == input::NAV_CANCEL) m_jukebox = false;
		else if (nav == input::NAV_START) { addSong(); sing(); }
		else if (event.menu == input::NAVMENU_B_NEXT)  m_audio.seek(-5);
		else if (event.menu == input::NAVMENU_B_PREV) m_audio.seek(5);
		else if (nav == input::NAV_MOREUP) m_audio.seek(-30);
		else if (nav == input::NAV_MOREDOWN) m_audio.seek(30);
	} else if (nav == input::NAV_CANCEL) {
		if (m_menuPos != 1) m_menuPos = 1;  // Exit menu (back to song selection)
		else if (!m_search.text.empty()) { m_search.text.clear(); m_songs.setFilter(m_search.text); }  // Clear search
		else if (m_songs.typeNum()) m_songs.typeChange(0);  // Clear type filter
		else gm->activateScreen("Intro");
	}
	// The rest are only available when there are songs available
	else if (m_songs.empty()) return;
	else if (nav == input::NAV_START) {
		if (m_menu.isOpen()) {
			m_menu.action();
		}
		else if (m_menuPos == 1 /* Cover browser */) {
			if (addSong()) sing();  // Add song and sing if it was the first to be added
		}
		else if (m_menuPos == 4) {
			m_menuPos = 1;
			m_jukebox = true;
		}
		else if (m_menuPos == 0 /* Playlist */) {
			if (gm->getCurrentPlayList().isEmpty()) {
				m_menuPos = 1;
				addSong();
			} else {
				createPlaylistMenu();
				m_menu.open();
			}
		}
	}
	else if (event.menu == input::NAVMENU_B_PREV) {
		if (m_menu.isOpen()) m_menu.move(-1);
		else if (m_menuPos < 4) ++m_menuPos;
	}
	else if (event.menu == input::NAVMENU_B_NEXT) {
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
		uint16_t mod = event.key.keysym.mod;
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
			// Shortcut keys for accessing different type filter modes
			if (key == SDL_SCANCODE_TAB) m_songs.sortChange(1);
			if (key == SDL_SCANCODE_F5) m_songs.typeCycle(2);
			if (key == SDL_SCANCODE_F6) m_songs.typeCycle(3);
			if (key == SDL_SCANCODE_F7) m_songs.typeCycle(4);
			if (key == SDL_SCANCODE_F8) m_songs.typeCycle(1);
		}
	}
	if (m_songs.empty()) m_jukebox = false;
}

void ScreenSongs::update() {
	Game* sm = Game::getSingletonPtr();
	sm->showLogo(!m_jukebox);
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
	Audio::aubioTempo.reset(new_aubio_tempo("default", Audio::aubio_win_size, Audio::aubio_hop_size, Audio::getSR()));
	}
	if (song && song->hasControllers()) { song->loadNotes(); } // Needed for BPM info.
	m_playing = music;
	// Clear the old content and load new content if available
	m_songbg.reset(); m_video.reset();
	double pstart = (!m_jukebox && song ? song->preview_start : 0.0);
	m_audio.playMusic(music, true, 1.0, pstart);
	if (song) {
		fs::path const& background = song->background.empty() ? song->cover : song->background;
		if (!background.empty()) try { m_songbg = std::make_unique<Texture>(background); } catch (std::exception const&) {}
		if (!song->video.empty() && config["graphic/video"].b()) m_video = std::make_unique<Video>(song->video, song->videoGap);
	}
}

bool ScreenSongs::addSong() {
	Game* gm = Game::getSingletonPtr();
	auto& pl = gm->getCurrentPlayList();
	bool empty = pl.getList().empty();
	pl.addSong(m_songs.currentPtr());
	return empty;
}

void ScreenSongs::sing() {
	Game* gm = Game::getSingletonPtr();
	ScreenSing& ss = dynamic_cast<ScreenSing&>(*gm->getScreen("Sing"));
	ss.setSong(gm->getCurrentPlayList().getNext());
	gm->activateScreen("Sing");
}

void ScreenSongs::prepare() {
	double time = m_audio.getPosition() - config["audio/video_delay"].f();
	if (m_video) m_video->prepare(time);
}

void ScreenSongs::drawJukebox() {
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
			s.dimensions.left(theme->song.dimensions.x1()).top(theme->song.dimensions.y2() + 0.05).fitInside(0.15, 0.15);
			s.draw();
		}
		// Format && draw the song information text
		std::ostringstream oss_song;
		oss_song << song.title << '\n' << song.artist;
		theme->song.draw(oss_song.str());
	}
}

void ScreenSongs::drawMultimedia() {
	if (!m_songs.empty()) {
		Transform ft(farTransform());  // 3D effect
		double length = m_audio.getLength();
		double time = clamp(m_audio.getPosition() - config["audio/video_delay"].f(), 0.0, length);
		m_songbg_default->draw();   // Default bg
		if (m_songbg.get() && !m_video.get()) {
			if (m_songbg->width() > 512 && m_songbg->dimensions.ar() > 1.1) {
				// Full screen mode
				float s = sin(m_clock.get()) * 0.15 + 1.15;
				Transform sc(glmath::scale(glmath::vec3(s, s, s)));
				m_songbg->draw();
			} else {
				// Low res texture or cover image, render in tiled mode
				double x = 0.05 * m_clock.get();
				m_songbg->draw(m_songbg->dimensions, TexCoords(x, 0.0, x + 5.0, 5.0));
			}
		}
		if (m_video.get()) m_video->render(time);
	}
	if (!m_jukebox) {
		m_songbg_ground->draw();
		theme->bg.draw();
		drawCovers();
	}
}

void ScreenSongs::draw() {
	update();
	drawMultimedia();
	std::ostringstream oss_song, oss_order, oss_hiscore;
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
		// Get hiscores from database
		m_database.queryPerSongHiscore(oss_hiscore, m_songs.currentPtr());
		// Escaped bytes of UTF-8 must be used here for compatibility with Windows (MSVC, mingw)
		char const* VERT_ARROW = "\xe2\x86\x95 ";  // ↕
		char const* HORIZ_ARROW = "\xe2\x86\x94 ";  // ↔
		char const* ENTER = "\xe2\x86\xb5 ";  // ↵
		char const* PAD = "   ";
		switch (m_menuPos) {
		case 1:
			if (!m_search.text.empty()) oss_order << m_search.text;
			else if (m_songs.typeNum()) oss_order << m_songs.typeDesc();
			else if (m_songs.sortNum()) oss_order << m_songs.sortDesc();
			else oss_order << _("<type in to search>") << PAD << HORIZ_ARROW << _("songs") << PAD << VERT_ARROW << _("options");
			break;
		case 2: oss_order << HORIZ_ARROW << _("sort order: ") << m_songs.sortDesc(); break;
		case 3: oss_order << HORIZ_ARROW << _("type filter: ") << m_songs.typeDesc(); break;
		case 4: oss_order << HORIZ_ARROW << _("hiscores") << PAD << ENTER << _("jukebox mode"); break;
		case 0:
			bool empty = Game::getSingletonPtr()->getCurrentPlayList().isEmpty();
			oss_order << ENTER << (empty ? _("start a playlist with this song!") : _("open the playlist menu"));
			break;
		}
	}

	if (m_jukebox) drawJukebox();
	else {
		// Draw song and order texts
		theme->song.draw(oss_song.str());
		theme->order.draw(oss_order.str());
		drawInstruments(Dimensions(1.0).fixedHeight(0.09).right(0.45).screenTop(0.02));
		theme->hiscores.draw(oss_hiscore.str());
	}
	// Menus on top of everything
	if (m_menu.isOpen()) drawMenu();
}

void ScreenSongs::drawCovers() {
	double spos = m_songs.currentPosition(); // This needs to be polled to run the animation
	std::size_t ss = m_songs.size();
	int currentId = m_songs.currentId();
	int baseidx = spos + 1.5; --baseidx; // Round correctly
	double shift = spos - baseidx;
	// Calculate beat
	double beat = 0.5 + m_idleTimer.get() / 2.0;  // 30 BPM
	if (ss > 0) {
		// Use actual song BPM. FIXME: Should only do this if currentId is also playing.
		if (m_songs.currentPtr()->music == m_playing) {
				if (m_songs.currentPtr()->hasControllers() || !m_songs.currentPtr()->beats.empty()) {
				double t = m_audio.getPosition() - config["audio/video_delay"].f();
				Song::Beats const& beats = m_songs.current().beats;
				auto it = std::lower_bound(m_songs.currentPtr()->hasControllers() ? beats.begin() : (beats.begin() + 1), beats.end(), t);
				if (it != beats.begin() && it != beats.end()) {
					double t1 = *(it - 1), t2 = *it;
					beat = (t - t1) / (t2 - t1);
				}
			}
			else if (m_songs.currentPtr() && !m_songs.currentPtr()->m_bpms.empty()) {
				double tempo = (m_songs.currentPtr()->m_bpms.front().step * 4.0);
				if (int(tempo) <= 100.0) tempo *= 2.0;
				else if (int(tempo) > 400.0) tempo /= 4.0;
				else if (int(tempo) > 300.0) tempo /= 3.0;
				else if (int(tempo) > 190.0) tempo /= 2.0;
				beat = 0.5 + m_idleTimer.get() / tempo;
			}
		}
	}
	beat = 1.0 + std::pow(std::abs(std::cos(0.5 * TAU * beat)), 10.0);  // Overdrive pulse
	// Draw covers and reflections
	for (int i = -2; i < 6; ++i) {
		if (baseidx + i < 0 || baseidx + i >= int(ss)) continue;
		Song& song = *m_songs[baseidx + i];
		Texture& s = getCover(song);
		// Calculate dimensions for cover and instrument markers
		double pos = i - shift;
		// Function for highlight effect (offset = 0 for current cover), returns 0..1 highlight level
		auto highlightf = [=](double offset) { return smoothstep(3.5, 0.0, std::abs(pos + offset)); };
		// Coordinate translations (pos and offset in cover units to z and x in OpenGL space)
		auto ztrans = [=](double offset) { return -0.5 + 0.3 * highlightf(offset); };
		auto xtrans = [=](double offset) { return -0.2 + 0.20 * (pos + offset); };
		// A cover is angled to a line between the surrounding gaps (offset +- 0.5 covers)
		double angle = -std::atan2(ztrans(0.5) - ztrans(-0.5), xtrans(0.5) - xtrans(-0.5));
		double y = 0.5 * virtH();
		double x = xtrans(0.0);
		double z = ztrans(0.0);
		double c = 0.4 + 0.6 * highlightf(0.0);
		if (m_menuPos == 1 /* Cover browser */ && baseidx + i == currentId) c = beat;
		using namespace glmath;
		Transform trans(translate(vec3(x, y, z)) * rotate(angle, vec3(0.0, 1.0, 0.0)));
		ColorTrans c1(Color(c, c, c));
		s.dimensions.middle().screenCenter().bottom().fitInside(0.17, 0.17);
		// Draw the cover normally
		s.draw();
		// Draw the reflection
		Transform transMirror(scale(vec3(1.0f, -1.0f, 1.0f)));
		ColorTrans c2(Color::alpha(0.4));
		s.draw();
	}
	// Draw the playlist
	Game* gm = Game::getSingletonPtr();
	auto const& playlist = gm->getCurrentPlayList().getList();
	double c = (m_menuPos == 0 /* Playlist */ ? beat : 1.0);
	ColorTrans c1(Color(c, c, c));
	for (unsigned i = playlist.size() - 1; i < playlist.size(); --i) {
		Texture& s = getCover(*playlist[i]);
		double pos =  i / std::max<double>(5.0, playlist.size());
		using namespace glmath;
		Transform trans(
		  translate(vec3(-0.35 + 0.06 * pos, 0.0, 0.3 - 0.2 * pos))
		  * rotate(-0.0, vec3(0.0, 1.0, 0.0))
		);
		s.dimensions.middle().screenBottom(-0.06).fitInside(0.08, 0.08);
		s.draw();
	}
}

Texture* ScreenSongs::loadTextureFromMap(fs::path path) {
	if(m_covers.find(path) == m_covers.end()) {
		std::pair<fs::path, std::unique_ptr<Texture>> kv = std::make_pair(path, std::make_unique<Texture>(path));
		m_covers.insert(std::move(kv));
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
		return (i-1)/float(iconcount);
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

	UseTexture tex(*m_instrumentList);
	double x = dim.x1();
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
		x -= (i > 0 ? 0.3 : 1.0) * dim.w();
	}
	for (int i = vocalCount-1; i >= 0; i--) {
		drawIcon(1, dim.left(x));
		x -= (i > 0 ? 0.3 : 1.0) * dim.w();
	}
}

void ScreenSongs::drawMenu() {
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
	for (MenuOptions::const_iterator it = m_menu.begin(); it != m_menu.end(); ++it) {
		// Pick the font object
		SvgTxtTheme* txt = &th.option_selected;
		if (cur != &*it)
			txt = &(th.getCachedOption(it->getName()));
		// Set dimensions and draw
		txt->dimensions.middle(x).center(y);
		txt->draw(it->getName());
		w = std::max(w, txt->w() + 2 * step); // Calculate the widest entry
		y += step;
	}
	if (cur->getComment() != "") {
		th.comment.dimensions.middle(0).screenBottom(-0.12);
		th.comment.draw(cur->getComment());
	}
	m_menu.dimensions.stretch(w, h);
}

std::unique_ptr<fvec_t, void(*)(fvec_t*)> ScreenSongs::previewSamplesBuffer = std::unique_ptr<fvec_t, void(*)(fvec_t*)>(new_fvec(1), [](fvec_t* p){del_fvec(p);});
std::unique_ptr<fvec_t, void(*)(fvec_t*)> ScreenSongs::previewBeatsBuffer = std::unique_ptr<fvec_t, void(*)(fvec_t*)>(new_fvec(1), [](fvec_t* p){del_fvec(p);});

void ScreenSongs::createPlaylistMenu() {
	m_menu.clear();
	m_menu.add(MenuOption(_("Play"), "").call([this]() {
		Game* tm = Game::getSingletonPtr();
		tm->getCurrentPlayList().addSong(m_songs.currentPtr());
		m_menuPos = 1;
		m_menu.close();
		sing();
	}));
	m_menu.add(MenuOption(_("Shuffle"), "").call([this]() {
		Game* tm = Game::getSingletonPtr();
		tm->getCurrentPlayList().shuffle();
		m_menuPos = 1;
		m_menu.close();
	}));
	m_menu.add(MenuOption(_("View playlist"), "").screen("Playlist"));
	m_menu.add(MenuOption(_("Clear playlist"), "").call([this]() {
		Game* tm = Game::getSingletonPtr();
		tm->getCurrentPlayList().clear();
		m_menuPos = 1;
		m_menu.close();
	}));
	m_menu.add(MenuOption(_("Back"), "").call([this]() {
		m_menuPos = 1;
		m_menu.close();
	}));
}
