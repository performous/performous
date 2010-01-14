#include "screen_songs.hh"

#include "screen_sing.hh"
#include "screen_hiscore.hh"
#include "configuration.hh"
#include "util.hh"
#include "songs.hh"
#include "audio.hh"
#include "i18n.hh"
#include <iostream>
#include <sstream>

static const double IDLE_TIMEOUT = 45.0; // seconds

ScreenSongs::ScreenSongs(std::string const& name, Audio& audio, Songs& songs, Database& database):
  Screen(name), m_audio(audio), m_songs(songs), m_database(database), m_covers(20), m_jukebox()
{
	m_songs.setAnimMargins(5.0, 5.0);
	m_playTimer.setTarget(getInf()); // Using this as a simple timer counting seconds
}

void ScreenSongs::enter() {
	theme.reset(new ThemeSongs());
	m_songbg_default.reset(new Surface(getThemePath("songs_bg_default.svg")));
	m_emptyCover.reset(new Surface(getThemePath("no_cover.svg")));
	m_instrumentCover.reset(new Surface(getThemePath("instrument_cover.svg")));
	m_bandCover.reset(new Surface(getThemePath("band_cover.svg")));
	m_instrumentList.reset(new Texture(getThemePath("instruments.svg")));
	m_songs.setFilter(m_search.text);
	m_audio.fadeout();
	m_jukebox = false;
}

void ScreenSongs::exit() {
	m_covers.clear();
	m_emptyCover.reset();
	m_instrumentCover.reset();
	m_bandCover.reset();
	m_instrumentList.reset();
	theme.reset();
	m_video.reset();
	m_songbg.reset();
	m_playing.clear();
	m_playReq.clear();
}

/**Add actions here which should effect both the
  jukebox and the normal screen*/
void ScreenSongs::manageSharedKey(input::NavButton nav) {
	if (nav == input::PAUSE) m_audio.togglePause();
	else if (nav == input::START) {
		ScreenManager* sm = ScreenManager::getSingletonPtr();
		Screen* s = sm->getScreen("Sing");
		ScreenSing* ss = dynamic_cast<ScreenSing*> (s);
		assert(ss);
		ss->setSong(m_songs.currentPtr());
		sm->activateScreen("Sing");
	}
	else if (nav == input::LEFT) m_songs.advance(-1);
	else if (nav == input::RIGHT) m_songs.advance(1);
}

void ScreenSongs::manageEvent(SDL_Event event) {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	input::NavButton nav(input::getNav(event));
	// Handle basic navigational input that is possible also with instruments
	if (nav != input::NONE) {
		if (m_jukebox) {
			if (nav == input::CANCEL || m_songs.empty()) m_jukebox = false;
			else if (nav == input::UP) m_audio.seek(5);
			else if (nav == input::DOWN) m_audio.seek(-5);
			else if (nav == input::MOREUP) m_audio.seek(-30);
			else if (nav == input::MOREDOWN) m_audio.seek(30);
			else manageSharedKey(nav);
			return;
		} else if (nav == input::CANCEL) {
			if (m_search.text.empty()) sm->activateScreen("Intro");
			else { m_search.text.clear(); m_songs.setFilter(m_search.text); }
		}
		// The rest are only available when there are songs available
		else if (m_songs.empty()) return;
		else if (nav == input::UP) m_songs.sortChange(-1);
		else if (nav == input::DOWN) m_songs.sortChange(1);
		else if (nav == input::MOREUP) m_songs.advance(-10);
		else if (nav == input::MOREDOWN) m_songs.advance(10);
		else if (nav == input::VOLUME_DOWN) --config["audio/preview_volume"];
		else if (nav == input::VOLUME_UP) ++config["audio/preview_volume"];
		else manageSharedKey(nav);
	// Handle less common, keyboard only keys
	} else if (event.type == SDL_KEYDOWN) {
		SDL_keysym keysym = event.key.keysym;
		int key = keysym.sym;
		SDLMod mod = event.key.keysym.mod;
		if (key == SDLK_r && mod & KMOD_CTRL) { m_songs.reload(); m_songs.setFilter(m_search.text); }
		if (!m_jukebox && m_search.process(keysym)) m_songs.setFilter(m_search.text);
		// The rest are only available when there are songs available
		else if (m_songs.empty()) return;
		else if (!m_jukebox && key == SDLK_F4) m_jukebox = true;
		else if (key == SDLK_END) {
			ScreenManager* sm = ScreenManager::getSingletonPtr();
			Screen* s = sm->getScreen("Hiscore");
			ScreenHiscore* ss = dynamic_cast<ScreenHiscore*> (s);
			assert(ss);
			ss->setSong(m_songs.currentPtr());
			sm->activateScreen("Hiscore");
		}
	}
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
		Surface& s = (cover ? *cover : *m_emptyCover);
		s.dimensions.right(theme->song.dimensions.x1()).top(theme->song.dimensions.y1()).fitInside(0.1, 0.1); s.draw();
		*/
		// Format && draw the song information text
		std::ostringstream oss_song;
		oss_song << song.title << '\n' << song.artist;
		theme->song.draw(oss_song.str());
	}
}

void ScreenSongs::drawMultimedia() {
	double length = m_audio.getLength();
	double time = clamp(m_audio.getPosition() - config["audio/video_delay"].f(), 0.0, length);
	if (m_songbg.get()) m_songbg->draw(); else m_songbg_default->draw();
	if (m_video.get()) m_video->render(time);
	if (!m_jukebox) theme->bg.draw();
}

void ScreenSongs::updateMultimedia(Song& song, ScreenSharedInfo& info) {
	if (!song.music.empty()) info.music = song.music; // TODO it is always empty?
	if (!song.background.empty()) info.songbg = song.path + song.background;
	if (!song.video.empty()) { info.video = song.path + song.video; info.videoGap = song.videoGap; }
}

void ScreenSongs::stopMultimedia(ScreenSharedInfo& info) {
	// Schedule playback change if the chosen song has changed
	if (info.music != m_playReq) { m_playReq = info.music; m_playTimer.setValue(0.0); }
	// Play/stop preview playback (if it is the time)
	if (info.music != m_playing && m_playTimer.get() > 0.3) {
		m_songbg.reset(); m_video.reset();
		double pstart = 0;
		if (!m_songs.empty() && !m_jukebox) {
			pstart = m_songs.current().preview_start;
			if (pstart != pstart) pstart = 100; // true if NaN, 100 is caught in the next min
			// 5.0s is for performance (don't make it higher unless you implement better seeking method in songs)
			pstart = std::min(pstart, (info.music.size() == 1 ? 30.0 : 5.0)); // we can seek further in 1-track songs
		}
		if (info.music.empty()) m_audio.fadeout(1.0); else m_audio.playMusic(info.music, true, 2.0, pstart);
		if (!info.songbg.empty()) try { m_songbg.reset(new Surface(info.songbg)); } catch (std::exception const&) {}
		if (!info.video.empty() && config["graphic/video"].b()) m_video.reset(new Video(info.video, info.videoGap));
		m_playing = info.music;
	}
}

namespace {
	float getIconTex(int i) {
		static int iconcount = 8;
		return (i-1)/float(iconcount);
	}
}

void ScreenSongs::draw() {
	m_songs.update(); // Poll for new songs
	ScreenSharedInfo info;
	info.videoGap = 0.0;

	drawMultimedia();
	std::ostringstream oss_song, oss_order, oss_hiscore;
	// Test if there are no songs
	if (m_songs.empty()) {
		// Format the song information text
		if (m_search.text.empty()) {
			oss_song << _("No songs found!");
			oss_order << _("Visit performous.org\nfor free songs");
		} else {
			oss_song << _("no songs match search");
			oss_order << m_search.text << '\n';
		}
	} else {
		Song& song = m_songs.current();
		// Format the song information text
		oss_song << song.title << '\n' << song.artist;
		if(m_database.hasHiscore(song))
			oss_hiscore << _("(press END to view hiscores)");
		oss_order << _("filter: ") << (m_search.text.empty() ? _("none") : m_search.text) << '\n';
		oss_order << m_songs.sortDesc() << '\n';
		oss_order << "(" << m_songs.currentId() + 1 << "/" << m_songs.size() << ")";
		double spos = m_songs.currentPosition(); // This needs to be polled to run the animation
		if (!m_jukebox) {
			// Draw the covers
			std::size_t ss = m_songs.size();
			int baseidx = spos + 1.5; --baseidx; // Round correctly
			double shift = spos - baseidx;
			for (int i = -2; i < 5; ++i) {
				if (baseidx + i < 0 || baseidx + i >= int(ss)) continue;
				Song& song_display = m_songs[baseidx + i];
				Surface* cover = NULL;
				// Fetch cover image from cache or try loading it
				if (!song_display.cover.empty()) try { cover = &m_covers[song_display.path + song_display.cover]; } catch (std::exception const&) {}
				if (!cover) {
					size_t tracks = song_display.track_map.size();
					if (tracks == 0) cover = m_emptyCover.get();
					else if (tracks == 1) cover = m_instrumentCover.get();
					else cover = m_bandCover.get();
				}
				Surface& s = *cover;
				double diff = (i == 0 ? (0.5 - fabs(shift)) * 0.07 : 0.0);
				double y = 0.27 + 0.5 * diff;
				// Draw the cover
				s.dimensions.middle(-0.2 + 0.17 * (i - shift)).bottom(y - 0.2 * diff).fitInside(0.14 + diff, 0.14 + diff); s.draw();
				// Draw the reflection
				s.dimensions.top(y + 0.2 * diff); s.tex = TexCoords(0, 1, 1, 0); glColor4f(1.0, 1.0, 1.0, 0.4); s.draw();
				s.tex = TexCoords(); glColor4f(1.0, 1.0, 1.0, 1.0); // Restore default attributes
				// Draw the intruments
				{
					UseTexture tex(*m_instrumentList);
					Dimensions dim = Dimensions(m_instrumentList->ar()).middle(-0.2 + 0.17 * (i - shift)).bottom(y - (0.14+diff) - 0.2 * diff).fitInside(0.14 + diff, 0.14 + diff);
					double x;
					float alpha;
					float xincr = 0.2f;
					{
						// vocals
						alpha = (song_display.notes.size()) ? 1.00 : 0.25;
						bool karaoke = (song_display.music.find("vocals") != song_display.music.end());
						glutil::Begin block(GL_TRIANGLE_STRIP);
						if(karaoke) {
							glColor4f(1.0, 1.0, 0.25, alpha);
						} else {
							glColor4f(1.0, 1.0, 1.0, alpha);
						}
						x = dim.x1()+0.00*(dim.x2()-dim.x1());
						glTexCoord2f(getIconTex(1), 0.0f); glVertex2f(x, dim.y1());
						glTexCoord2f(getIconTex(1), 1.0f); glVertex2f(x, dim.y2());
						x = dim.x1()+xincr*(dim.x2()-dim.x1());
						glTexCoord2f(getIconTex(2), 0.0f); glVertex2f(x, dim.y1());
						glTexCoord2f(getIconTex(2), 1.0f); glVertex2f(x, dim.y2());
					}
					{
						// guitars
						alpha = 1.0;
						int guitarCount = 0;
						if (isTrackInside(song_display.track_map,"guitar")) guitarCount++;
						if (isTrackInside(song_display.track_map,"coop guitar")) guitarCount++;
						if (isTrackInside(song_display.track_map,"rhythm guitar")) guitarCount++;
						if (guitarCount == 0) { guitarCount = 1; alpha = 0.25; }
						for (int i = guitarCount-1; i >= 0; i--) {
							glutil::Begin block(GL_TRIANGLE_STRIP);
							glColor4f(1.0, 1.0, 1.0, alpha);
							x = dim.x1()+(xincr+i*0.04)*(dim.x2()-dim.x1());
							glTexCoord2f(getIconTex(2), 0.0f); glVertex2f(x, dim.y1());
							glTexCoord2f(getIconTex(2), 1.0f); glVertex2f(x, dim.y2());
							x = dim.x1()+(2*xincr+i*0.04)*(dim.x2()-dim.x1());
							glTexCoord2f(getIconTex(3), 0.0f); glVertex2f(x, dim.y1());
							glTexCoord2f(getIconTex(3), 1.0f); glVertex2f(x, dim.y2());
						}
					}
					{
						// bass
						alpha = (isTrackInside(song_display.track_map,"bass")) ? 1.00 : 0.25;
						glutil::Begin block(GL_TRIANGLE_STRIP);
						glColor4f(1.0, 1.0, 1.0, alpha);
						x = dim.x1()+2*xincr*(dim.x2()-dim.x1());
						glTexCoord2f(getIconTex(3), 0.0f); glVertex2f(x, dim.y1());
						glTexCoord2f(getIconTex(3), 1.0f); glVertex2f(x, dim.y2());
						x = dim.x1()+3*xincr*(dim.x2()-dim.x1());
						glTexCoord2f(getIconTex(4), 0.0f); glVertex2f(x, dim.y1());
						glTexCoord2f(getIconTex(4), 1.0f); glVertex2f(x, dim.y2());
					}
					{
						// drums
						alpha = (isTrackInside(song_display.track_map,"drums")) ? 1.00 : 0.25;
						glutil::Begin block(GL_TRIANGLE_STRIP);
						glColor4f(1.0, 1.0, 1.0, alpha);
						x = dim.x1()+3*xincr*(dim.x2()-dim.x1());
						glTexCoord2f(getIconTex(4), 0.0f); glVertex2f(x, dim.y1());
						glTexCoord2f(getIconTex(4), 1.0f); glVertex2f(x, dim.y2());
						x = dim.x1()+4*xincr*(dim.x2()-dim.x1());
						glTexCoord2f(getIconTex(5), 0.0f); glVertex2f(x, dim.y1());
						glTexCoord2f(getIconTex(5), 1.0f); glVertex2f(x, dim.y2());
					}
					{
						// dancing
						alpha = !song_display.danceTracks.empty() ? 1.00 : 0.25;
						glutil::Begin block(GL_TRIANGLE_STRIP);
						glColor4f(1.0, 1.0, 1.0, alpha);
						x = dim.x1()+4*xincr*(dim.x2()-dim.x1());
						glTexCoord2f(getIconTex(5), 0.0f); glVertex2f(x, dim.y1());
						glTexCoord2f(getIconTex(5), 1.0f); glVertex2f(x, dim.y2());
						x = dim.x1()+5*xincr*(dim.x2()-dim.x1());
						glTexCoord2f(getIconTex(6), 0.0f); glVertex2f(x, dim.y1());
						glTexCoord2f(getIconTex(6), 1.0f); glVertex2f(x, dim.y2());
					}
					glColor4f(1.0, 1.0, 1.0, 1.0);
				}
			}
		}
		updateMultimedia(song, info);
	}
	if (m_jukebox) drawJukebox();
	else {
		// Draw song and order texts
		theme->song.draw(oss_song.str());
		theme->order.draw(oss_order.str());
		theme->hiscore.draw(oss_hiscore.str());
	}
	stopMultimedia(info);
	if (m_jukebox) {
		// Switch if at song end
		if (!m_audio.isPlaying() || m_audio.getPosition() + 1.3 > m_audio.getLength()) {
			m_songs.advance(1);
			// Force reload of data
			m_playing.clear();
		}
	} else if (!m_audio.isPaused() && m_playTimer.get() > IDLE_TIMEOUT) m_songs.advance(1);  // Switch if song hasn't changed for IDLE_TIMEOUT seconds
}

