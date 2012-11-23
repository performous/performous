#include "screen_songs.hh"

#include "screen_sing.hh"
#include "configuration.hh"
#include "hiscore.hh"
#include "util.hh"
#include "songs.hh"
#include "audio.hh"
#include "i18n.hh"
#include <iostream>
#include <sstream>
#include <boost/format.hpp>

static const double IDLE_TIMEOUT = 45.0; // seconds

ScreenSongs::ScreenSongs(std::string const& name, Audio& audio, Songs& songs, Database& database):
  Screen(name), m_audio(audio), m_songs(songs), m_database(database), m_covers(20)
{
	m_songs.setAnimMargins(5.0, 5.0);
	m_idleTimer.setTarget(getInf()); // Using this as a simple timer counting seconds
}

void ScreenSongs::enter() {
	m_songs.setFilter(m_search.text);
	m_audio.fadeout();
	m_menuPos = 0;
	m_infoPos = 0;
	m_jukebox = false;
	reloadGL();
}

void ScreenSongs::reloadGL() {
	theme.reset(new ThemeSongs());
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
		case 0: m_songs.advance(dir); break;
		case 1: m_songs.sortChange(dir); break;
		case 2: m_songs.typeChange(dir); break;
		case 3: m_infoPos = (m_infoPos + dir + 5) % 5; break;
	}
}

void ScreenSongs::manageEvent(SDL_Event event) {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	input::NavButton nav(input::getNav(event));
	if (nav != input::NONE) {  // Handle generic navigation (also possible with instruments)
		m_idleTimer.setValue(0.0);  // Reset idle timer
		if (nav == input::PAUSE) m_audio.togglePause();
		else if (m_jukebox) {
			if (nav == input::CANCEL) m_jukebox = false;
			else if (nav == input::UP) m_audio.seek(5);
			else if (nav == input::DOWN) m_audio.seek(-5);
			else if (nav == input::MOREUP) m_audio.seek(-30);
			else if (nav == input::MOREDOWN) m_audio.seek(30);
		} else if (nav == input::CANCEL) {
			if (m_menuPos) m_menuPos = 0;  // Exit menu (back to song selection)
			else if (!m_search.text.empty()) { m_search.text.clear(); m_songs.setFilter(m_search.text); }  // Clear search
			else if (m_songs.typeNum()) m_songs.typeChange(0);  // Clear type filter
			else sm->activateScreen("Intro");
		}
		// The rest are only available when there are songs available
		else if (m_songs.empty()) return;
		else if (nav == input::START) {
			if (m_menuPos == 0) {
				dynamic_cast<ScreenSing&>(*sm->getScreen("Sing")).setSong(m_songs.currentPtr());
				sm->activateScreen("Sing");
			}
			else if (m_menuPos == 3) {
				m_menuPos = 0;
				m_jukebox = true;
			}
		}
		else if (nav == input::LEFT) menuBrowse(-1);
		else if (nav == input::RIGHT) menuBrowse(1);
		else if (nav == input::UP && m_menuPos < 3) ++m_menuPos;
		else if (nav == input::DOWN && m_menuPos > 0) --m_menuPos;
		else if (nav == input::MOREUP) m_songs.advance(-10);
		else if (nav == input::MOREDOWN) m_songs.advance(10);
	} else if (event.type == SDL_KEYDOWN) {  // Handle keyboard-only navigation
		SDL_keysym keysym = event.key.keysym;
		int key = keysym.sym;
		SDLMod mod = event.key.keysym.mod;
		if (key == SDLK_F4) m_jukebox = !m_jukebox;
		else if (!m_jukebox) {
			if (key == SDLK_r && mod & KMOD_CTRL) { m_songs.reload(); m_songs.setFilter(m_search.text); }
			else if (m_search.process(keysym)) m_songs.setFilter(m_search.text);
		}
	}
	if (m_songs.empty()) m_jukebox = false;
	sm->showLogo(!m_jukebox);
}

void ScreenSongs::update() {
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
	m_playing = music;
	// Clear the old content and load new content if available
	m_songbg.reset(); m_video.reset();
	double pstart = (!m_jukebox && song ? song->preview_start : 0.0);
	m_audio.playMusic(music, true, 2.0, pstart);
	if (song) {
		std::string background = song->background;
		std::string video = song->video;
		if (!background.empty()) try { m_songbg.reset(new Surface(song->path + background)); } catch (std::exception const&) {}
		if (!video.empty() && config["graphic/video"].b()) m_video.reset(new Video(song->path + video, song->videoGap));
	}
}

void ScreenSongs::prepare() {
	double time = m_audio.getPosition();
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
		/*
		Surface* cover = NULL;
		if (!song.cover.empty()) try { cover = &m_covers[song.path + song.cover]; } catch (std::exception const&) {}
		Surface& s = (cover ? *cover : *m_singCover);
		s.dimensions.right(theme->song.dimensions.x1()).top(theme->song.dimensions.y1()).fitInside(0.1, 0.1); s.draw();
		*/
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

namespace {
	float getIconTex(int i) {
		static int iconcount = 8;
		return (i-1)/float(iconcount);
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
		oss_song << song.title << '\n' << song.artist;
		oss_order << (m_search.text.empty() ? _("<type in to search>") : m_search.text) << "\n\n";
		// Format the song information text
		oss_hiscore << boost::format(_("Hisccore for %1%\n")) % song.title;
		// Get hiscores from database
		m_database.queryPerSongHiscore_HiscoreDisplay(oss_hiscore, m_songs.currentPtr(), m_infoPos, 5);
	}
	switch (m_menuPos) {
		case 0: if (!m_songs.empty()) oss_order << "(" << m_songs.currentId() + 1 << "/" << m_songs.size() << ")"; break;
		case 1: oss_order << _("↔ sort order: ") << m_songs.sortDesc(); break;
		case 2: oss_order << _("↔ type filter: ") << m_songs.typeDesc(); break;
		case 3: oss_order << _("↔ hiscores   ↵ jukebox mode"); break;
	}	

	if (m_jukebox) drawJukebox();
	else {
		// Draw song and order texts
		theme->song.draw(oss_song.str());
		theme->order.draw(oss_order.str());
		drawInstruments(Dimensions(m_instrumentList->ar()).fixedHeight(0.03).center(-0.04));
		using namespace glmath;
		Transform trans(translate(vec3(0.25, 0.0, 0.0)) * scale(vec3(0.2, 1.0, 1.0)));
		theme->hiscores.draw(oss_hiscore.str());
	}
}

void ScreenSongs::drawCovers() {
	double spos = m_songs.currentPosition(); // This needs to be polled to run the animation
	std::size_t ss = m_songs.size();
	int baseidx = spos + 1.5; --baseidx; // Round correctly
	double shift = spos - baseidx;
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
		ColorTrans c1(Color(c, c, c));
		s.dimensions.middle(0.0).bottom(0.0).fitInside(0.17, 0.17);
		// Draw the cover normally
		s.draw();
		// Draw the reflection
		Transform transMirror(scale(vec3(1.0f, -1.0f, 1.0f)));
		ColorTrans c2(Color::alpha(0.4));
		s.draw();
	}
}

Surface& ScreenSongs::getCover(Song const& song) {
	Surface* cover = NULL;
	// Fetch cover image from cache or try loading it
	if (!song.cover.empty()) try { cover = &m_covers[song.path + song.cover]; } catch (std::exception const&) {cover = NULL;}
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

void ScreenSongs::drawInstruments(Dimensions const& dim, float alpha) const {

	bool have_vocals = false;
	bool have_bass = false;
	bool have_drums = false;
	bool have_keyboard = false;
	bool have_dance = false;
	bool is_karaoke = false;
	unsigned char typeFilter = 0;  // FIXME: Remove
	int guitarCount = 0;
	int vocalCount = 0;

	if( !m_songs.empty() ) {
		Song const& song = m_songs.current();
		have_vocals = song.hasVocals();
		have_bass = isTrackInside(song.instrumentTracks,TrackName::BASS);
		have_drums = song.hasDrums();
		have_keyboard = song.hasKeyboard();
		have_dance = song.hasDance();
		is_karaoke = (song.music.find("vocals") != song.music.end());
		vocalCount = song.getVocalTrackNames().size();
		if (isTrackInside(song.instrumentTracks,TrackName::GUITAR)) guitarCount++;
		if (isTrackInside(song.instrumentTracks,TrackName::GUITAR_COOP)) guitarCount++;
		if (isTrackInside(song.instrumentTracks,TrackName::GUITAR_RHYTHM)) guitarCount++;
	}

	UseTexture tex(*m_instrumentList);
	double x;
	float xincr = 0.2f;
	{
		// vocals
		float a = alpha;
		float m = !(typeFilter & 8);
		if (vocalCount == 0) { vocalCount = 1; a *= 0.25f; }
		for (int i = vocalCount-1; i >= 0; i--) {
			glutil::VertexArray va;
			glmath::vec4 c(m * a, a, m * (is_karaoke ? 0.25f : 1.0f) * a, a);
			x = dim.x1()+(i*0.03)*(dim.x2()-dim.x1());
			va.Color(c).TexCoord(getIconTex(1), 0.0f).Vertex(x, dim.y1());
			va.Color(c).TexCoord(getIconTex(1), 1.0f).Vertex(x, dim.y2());
			x = dim.x1()+(xincr+i*0.03)*(dim.x2()-dim.x1());
			va.Color(c).TexCoord(getIconTex(2), 0.0f).Vertex(x, dim.y1());
			va.Color(c).TexCoord(getIconTex(2), 1.0f).Vertex(x, dim.y2());
			va.Draw();
		}
	}
	{
		// guitars
		float a = alpha;
		float m = !(typeFilter & 4);
		if (guitarCount == 0) { guitarCount = 1; a *= 0.25f; }
		for (int i = guitarCount-1; i >= 0; i--) {
			glutil::VertexArray va;
			glmath::vec4 c(m * a, a, m * a, a);
			x = dim.x1()+(xincr+i*0.04)*(dim.x2()-dim.x1());
			va.Color(c).TexCoord(getIconTex(2), 0.0f).Vertex(x, dim.y1());
			va.Color(c).TexCoord(getIconTex(2), 1.0f).Vertex(x, dim.y2());
			x = dim.x1()+(2*xincr+i*0.04)*(dim.x2()-dim.x1());
			va.Color(c).TexCoord(getIconTex(3), 0.0f).Vertex(x, dim.y1());
			va.Color(c).TexCoord(getIconTex(3), 1.0f).Vertex(x, dim.y2());
			va.Draw();
		}
	}
	{
		// bass
		float a = alpha * (have_bass ? 1.00f : 0.25f);
		float m = !(typeFilter & 4);
		glutil::VertexArray va;
		glmath::vec4 c(m * a, a, m * a, a);
		x = dim.x1()+2*xincr*(dim.x2()-dim.x1());
		va.Color(c).TexCoord(getIconTex(3), 0.0f).Vertex(x, dim.y1());
		va.Color(c).TexCoord(getIconTex(3), 1.0f).Vertex(x, dim.y2());
		x = dim.x1()+3*xincr*(dim.x2()-dim.x1());
		va.Color(c).TexCoord(getIconTex(4), 0.0f).Vertex(x, dim.y1());
		va.Color(c).TexCoord(getIconTex(4), 1.0f).Vertex(x, dim.y2());
		va.Draw();
	}
	{
		// drums
		float a = alpha * (have_drums ? 1.00f : 0.25f);
		float m = !(typeFilter & 2);
		glutil::VertexArray va;
		glmath::vec4 c(m * a, a, m * a, a);
		x = dim.x1()+3*xincr*(dim.x2()-dim.x1());
		va.Color(c).TexCoord(getIconTex(4), 0.0f).Vertex(x, dim.y1());
		va.Color(c).TexCoord(getIconTex(4), 1.0f).Vertex(x, dim.y2());
		x = dim.x1()+4*xincr*(dim.x2()-dim.x1());
		va.Color(c).TexCoord(getIconTex(5), 0.0f).Vertex(x, dim.y1());
		va.Color(c).TexCoord(getIconTex(5), 1.0f).Vertex(x, dim.y2());
		va.Draw();
	}
	/*{
		// keyboard
		float a = alpha * (have_keyboard ? 1.00f : 0.25f);
		float m = !(typeFilter & 16);
		glutil::VertexArray va;
		glmath::vec4 c(m * 1.0f, 1.0f, m * 1.0f, a);
		x = dim.x1()+4*xincr*(dim.x2()-dim.x1());
		va.Color(c).TexCoord(getIconTex(5), 0.0f).Vertex(x, dim.y1());
		va.Color(c).TexCoord(getIconTex(5), 1.0f).Vertex(x, dim.y2());
		x = dim.x1()+5*xincr*(dim.x2()-dim.x1());
		va.Color(c).TexCoord(getIconTex(6), 0.0f).Vertex(x, dim.y1());
		va.Color(c).TexCoord(getIconTex(6), 1.0f).Vertex(x, dim.y2());
		va.Draw();
	}*/
	{
		// dancing
		float a = alpha * (have_dance ? 1.00f : 0.25f);
		float m = !(typeFilter & 1);
		glutil::VertexArray va;
		glmath::vec4 c(m * a, a, m * a, a);
		x = dim.x1()+4*xincr*(dim.x2()-dim.x1());
		va.Color(c).TexCoord(getIconTex(6), 0.0f).Vertex(x, dim.y1());
		va.Color(c).TexCoord(getIconTex(6), 1.0f).Vertex(x, dim.y2());
		x = dim.x1()+5*xincr*(dim.x2()-dim.x1());
		va.Color(c).TexCoord(getIconTex(7), 0.0f).Vertex(x, dim.y1());
		va.Color(c).TexCoord(getIconTex(7), 1.0f).Vertex(x, dim.y2());
		va.Draw();
	}
}

