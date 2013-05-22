#include "screen_songs.hh"

#include "audio.hh"
#include "configuration.hh"
#include "database.hh"
#include "hiscore.hh"
#include "i18n.hh"
#include "screen_sing.hh"
#include "screen_playlist.hh"
#include "songs.hh"
#include "theme.hh"
#include "util.hh"
#include "playlist.hh"
#include <iostream>
#include <sstream>
#include <boost/format.hpp>

static const double IDLE_TIMEOUT = 45.0; // seconds

ScreenSongs::ScreenSongs(std::string const& name, Audio& audio, Songs& songs, Database& database):
  Screen(name), m_audio(audio), m_songs(songs), m_database(database), m_covers(50)
{
	m_songs.setAnimMargins(5.0, 5.0);
	m_idleTimer.setTarget(getInf()); // Using this as a simple timer counting seconds
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
	theme.reset(new ThemeSongs());
	m_menuTheme.reset(new ThemeInstrumentMenu());
	m_songbg_default.reset(new Surface(getThemePath("songs_bg_default.svg")));
	m_songbg_ground.reset(new Surface(getThemePath("songs_bg_ground.svg")));
	m_singCover.reset(new Surface(getThemePath("no_cover.svg")));
	m_instrumentCover.reset(new Surface(getThemePath("instrument_cover.svg")));
	m_bandCover.reset(new Surface(getThemePath("band_cover.svg")));
	m_danceCover.reset(new Surface(getThemePath("dance_cover.svg")));
	m_instrumentList.reset(new Texture(getThemePath("instruments.svg")));
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
	if (event.type == SDL_KEYDOWN) {
		SDL_keysym keysym = event.key.keysym;
		int key = keysym.sym;
		SDLMod mod = event.key.keysym.mod;
		if (key == SDLK_F4) m_jukebox = !m_jukebox;
		else if (!m_jukebox) {
			if (key == SDLK_r && mod & KMOD_CTRL) { m_songs.reload(); m_songs.setFilter(m_search.text); }
			else if (m_search.process(keysym)) m_songs.setFilter(m_search.text);
			// Shortcut keys for accessing different type filter modes
			if (key == SDLK_TAB) m_songs.sortChange(1);
			if (key == SDLK_F5) m_songs.typeCycle(1);
			if (key == SDLK_F6) m_songs.typeCycle(2);
			if (key == SDLK_F7) m_songs.typeCycle(3);
			if (key == SDLK_F8) m_songs.typeCycle(4);
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
	boost::shared_ptr<Song> song = m_songs.currentPtr();
	Song::Music music;
	if (song) music = song->music;
	if (m_playing != music) songChange = true;
	// Switch songs if needed, only when the user is not browsing for a moment
	if (!songChange) return;
	if (song) song->loadNotes();  // Needed for BPM info; TODO: drop notes when switching to another song.
	m_playing = music;
	// Clear the old content and load new content if available
	m_songbg.reset(); m_video.reset();
	double pstart = (!m_jukebox && song ? song->preview_start : 0.0);
	m_audio.playMusic(music, true, 2.0, pstart);
	if (song) {
		std::string background = song->background.empty() ? song->cover : song->background;
		std::string video = song->video;
		if (!background.empty()) try { m_songbg.reset(new Surface(song->path + background)); } catch (std::exception const&) {}
		if (!video.empty() && config["graphic/video"].b()) m_video.reset(new Video(song->path + video, song->videoGap));
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
		Surface* cover = NULL;
		if (!song.cover.empty()) try { cover = &m_covers[song.path + song.cover]; } catch (std::exception const&) {}
		if (cover) {
			Surface& s = *cover;
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
	{
		Transform ft(farTransform());  // 3D effect
		double length = m_audio.getLength();
		double time = clamp(m_audio.getPosition() - config["audio/video_delay"].f(), 0.0, length);
		m_songbg_default->draw();   // Default bg
		if (!m_songs.empty()) {
			Song& song = m_songs.current();
			if (m_songbg.get()) m_songbg->draw();
			else if (!song.cover.empty()) {
				// Create a background image by tiling covers
				try {
					Surface& cover = m_covers[song.path + song.cover];
					Dimensions backup = cover.dimensions;
					const float s = 0.3;
					cover.dimensions.fixedWidth(s).screenTop(0.0);
					cover.dimensions.top(0.0).right(-s); cover.draw();
					cover.dimensions.top(0.0).right(0.0); cover.draw();
					cover.dimensions.top(0.0).left(0.0); cover.draw();
					cover.dimensions.top(0.0).left(s); cover.draw();
					cover.dimensions.top(s).right(-s); cover.draw();
					cover.dimensions.top(s).right(0.0); cover.draw();
					cover.dimensions.top(s).left(0.0); cover.draw();
					cover.dimensions.top(s).left(s); cover.draw();
					cover.dimensions = backup;
				} catch (std::exception const&) {}
			}
			if (m_video.get()) m_video->render(time);
		}
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
		if (m_search.text.empty() && !m_songs.typeNum()) {
			oss_song << _("No songs found!");
			oss_order << _("Visit performous.org\nfor free songs");
		} else {
			oss_song << _("no songs match search");
			oss_order << m_search.text << "\n\n";
		}
	} else {
		Song& song = m_songs.current();
		// Format the song information text
		oss_song << song.artist << ": " << song.title;
		// Format the song information text
		oss_hiscore << _("Hiscore\n");
		// Get hiscores from database
		m_database.queryPerSongHiscore_HiscoreDisplay(oss_hiscore, m_songs.currentPtr(), m_infoPos, 5);
	}
	switch (m_menuPos) {
		case 1:
			if (!m_search.text.empty()) oss_order << m_search.text;
			else if (m_songs.typeNum()) oss_order << m_songs.typeDesc();
			else if (m_songs.sortNum()) oss_order << m_songs.sortDesc();
			else oss_order << _("<type in to search>   ↔ songs    ↕ options");
			//if (!m_songs.empty()) oss_order << "(" << m_songs.currentId() + 1 << "/" << m_songs.size() << ")";
			break;
		case 2: oss_order << _("↔ sort order: ") << m_songs.sortDesc(); break;
		case 3: oss_order << _("↔ type filter: ") << m_songs.typeDesc(); break;
		case 4: oss_order << _("↔ hiscores   ↵ jukebox mode"); break;
		case 0:
	    Game* gm = Game::getSingletonPtr();
		if(gm->getCurrentPlayList().isEmpty()) {
			oss_order << _("↵ start a playlist with this song!");
		} else {
			oss_order << _("↵ open the playlist menu");
		}
		break;
	}	

	if (m_jukebox) drawJukebox();
	else {
		// Draw song and order texts
		theme->song.draw(oss_song.str());
		theme->order.draw(oss_order.str());
		drawInstruments(Dimensions(1.0).fixedHeight(0.03).middle(-0.22).screenBottom(-0.01));
		using namespace glmath;
		Transform trans(translate(vec3(0.32, -0.03, 0.0)) * scale(vec3(0.75, 0.75, 1.0)));
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
	double beat = 0.5 + m_idleTimer.get() * 2.0;  // 120 BPM
	if (ss > 0) {
		// Use actual song BPM. FIXME: Should only do this if currentId is also playing.
		double t = m_audio.getPosition() - config["audio/video_delay"].f();
		Song::Beats const& beats = m_songs[currentId].beats;
		auto it = std::lower_bound(beats.begin(), beats.end(), t);
		if (it != beats.begin() && it != beats.end()) {
			double t1 = *(it - 1), t2 = *it;
			beat = (t - t1) / (t2 - t1);
		}
	}
	beat = 1.0 + std::pow(std::abs(std::cos(3.141592 * beat)), 10.0);  // Overdrive pulse
	// Draw covers and reflections
	for (int i = -2; i < 6; ++i) {
		if (baseidx + i < 0 || baseidx + i >= int(ss)) continue;
		Song& song = m_songs[baseidx + i];
		Surface& s = getCover(song);
		// Calculate dimensions for cover and instrument markers
		double diff = 0.5 * (1.0 + std::cos(std::min(M_PI, std::abs(i - shift))));  // 0..1 for current cover hilight level
		double y = 0.5 * virtH();
		using namespace glmath;
		Transform trans(
		  translate(vec3(-0.2 + 0.20 * (i - shift), y, -0.2 - 0.3 * (1.0 - diff)))
		  * rotate(0.4 * std::sin(std::min(M_PI, i - shift)), vec3(0.0, 1.0, 0.0))
		);
		double c = 0.4 + 0.6 * diff;
		if (m_menuPos == 1 /* Cover browser */ && baseidx + i == currentId) c = beat;
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
		Surface& s = getCover(*playlist[i]);
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

Surface& ScreenSongs::getCover(Song const& song) {
	Surface* cover = nullptr;
	// Fetch cover image from cache or try loading it
	if (!song.cover.empty()) try { cover = &m_covers[song.path + song.cover]; } catch (std::exception const&) {cover = nullptr;}
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
		va.TexCoord(getIconTex(i), 0.0f).Vertex(dim.x1(), dim.y1());
		va.TexCoord(getIconTex(i), 1.0f).Vertex(dim.x1(), dim.y2());
		va.TexCoord(getIconTex(i + 1), 0.0f).Vertex(dim.x2(), dim.y1());
		va.TexCoord(getIconTex(i + 1), 1.0f).Vertex(dim.x2(), dim.y2());
		va.Draw();
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
		have_bass = isTrackInside(song.instrumentTracks,TrackName::BASS);
		have_drums = song.hasDrums();
		have_dance = song.hasDance();
		//is_karaoke = (song.music.find("vocals") != song.music.end());
		vocalCount = song.getVocalTrackNames().size();
		if (isTrackInside(song.instrumentTracks,TrackName::GUITAR)) guitarCount++;
		if (isTrackInside(song.instrumentTracks,TrackName::GUITAR_COOP)) guitarCount++;
		if (isTrackInside(song.instrumentTracks,TrackName::GUITAR_RHYTHM)) guitarCount++;
	}

	UseTexture tex(*m_instrumentList);
	double x = dim.x1();
	for (int i = vocalCount-1; i >= 0; i--) {
		drawIcon(1, dim.left(x));
		x += (i > 0 ? 0.3 : 1.0) * dim.w();
	}
	// guitars
	for (int i = guitarCount-1; i >= 0; i--) {
		drawIcon(2, dim.left(x));
		x += (i > 0 ? 0.3 : 1.0) * dim.w();
	}
	// bass
	if (have_bass) {
		drawIcon(3, dim.left(x));
		x += dim.w();
	}
	// drums
	if (have_drums) {
		drawIcon(4, dim.left(x));
		x += dim.w();
	}
	// dancing
	if (have_dance) {
		drawIcon(6, dim.left(x));
		x += dim.w();
	}
}

void ScreenSongs::drawMenu() {
	if (m_menu.empty()) return;
	// Some helper vars
	ThemeInstrumentMenu& th = *m_menuTheme;
	auto cur = static_cast<MenuOptions::const_iterator>(&m_menu.current());
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
		if (cur != it) txt = &(th.getCachedOption(it->getName()));
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

void ScreenSongs::createPlaylistMenu() {
	m_menu.clear();
	m_menu.add(MenuOption(_("Play"), _("Start the game with all songs in playlist")).call([this]() {
		Game* tm = Game::getSingletonPtr();
		tm->getCurrentPlayList().addSong(m_songs.currentPtr());
		m_menuPos = 1;
		m_menu.close();
		sing();
	}));
	m_menu.add(MenuOption(_("Shuffle"), _("Randomize the order of the playlist")).call([this]() {
		Game* tm = Game::getSingletonPtr();
		tm->getCurrentPlayList().shuffle();
		m_menuPos = 1;
		m_menu.close();
	}));
	m_menu.add(MenuOption(_("View playlist"), _("View the current playlist")).screen("Playlist"));
	m_menu.add(MenuOption(_("Clear playlist"), _("Remove all the songs from the list")).call([this]() {
		Game* tm = Game::getSingletonPtr();
		tm->getCurrentPlayList().clear();
		m_menuPos = 1;
		m_menu.close();
	}));
	m_menu.add(MenuOption(_("Back"), _("Back to song browser")).call([this]() {
		m_menuPos = 1;
		m_menu.close();
	}));
}
