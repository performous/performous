#include "screen_players.hh"
#include "screen_songs.hh"

#include "configuration.hh"
#include "audio.hh"
#include "database.hh"
#include "fs.hh"
#include "util.hh"
#include "layout_singer.hh"
#include "theme.hh"
#include "video.hh"
#include "i18n.hh"
#include "controllers.hh"
#include "notegraphscalerfactory.hh"

#include <iostream>
#include <sstream>
#include <boost/format.hpp>

ScreenPlayers::ScreenPlayers(std::string const& name, Audio& audio, Database& database):
  Screen(name), m_audio(audio), m_database(database), m_players(database.m_players)
{
	m_players.setAnimMargins(5.0, 5.0);
	m_playTimer.setTarget(getInf()); // Using this as a simple timer counting seconds
}

void ScreenPlayers::enter() {
	keyPressed = false;
	const auto scaler = NoteGraphScalerFactory().create();
	m_layout_singer = std::make_unique<LayoutSinger>(m_song->getVocalTrack(0), m_database, scaler);
	theme = std::make_unique<ThemeSongs>();
	m_emptyCover = std::make_unique<Texture>(findFile("no_player_image.svg"));
	m_search.text.clear();
	m_players.setFilter(m_search.text);
	m_audio.fadeout();
	m_quitTimer.setValue(config["game/highscore_timeout"].i());
	if (m_database.scores.empty() || !m_database.reachedHiscore(m_song)) {
		Game::getSingletonPtr()->activateScreen("Playlist");
	}
}

void ScreenPlayers::exit() {
	m_layout_singer.reset();

	m_covers.clear();
	m_emptyCover.reset();
	theme.reset();
	m_video.reset();
	m_songbg.reset();
	m_playing.clear();
	m_playReq.clear();

	m_database.save();
}

void ScreenPlayers::manageEvent(input::NavEvent const& event) {
	keyPressed = true;
	Game* gm = Game::getSingletonPtr();
	input::NavButton nav = event.button;
	if (nav == input::NAV_CANCEL) {
		if (m_search.text.empty()) { gm->activateScreen("Songs"); return; }
		else { m_search.text.clear(); m_players.setFilter(m_search.text); }
	} else if (nav == input::NAV_START) {
		if (m_players.empty()) {
			m_players.addPlayer(m_search.text);
			m_players.setFilter(m_search.text);
			m_players.update();
			// the current player is the new created one
		}
		m_database.addHiscore(m_song);
		m_database.scores.pop_front();

		if (m_database.scores.empty() || !m_database.reachedHiscore(m_song)) {
			// no more highscore, we are now finished
			gm->activateScreen("Playlist");
		} else {
			m_search.text.clear();
			m_players.setFilter("");
			// add all players which reach highscore because if score is very near or same it might be
			// frustrating for second one that he cannot enter, so better go for next one...
		}
	}
	else if (m_players.empty()) return;
	else if (nav == input::NAV_PAUSE) m_audio.togglePause();
	else if (nav == input::NAV_LEFT) m_players.advance(-1);
	else if (nav == input::NAV_RIGHT) m_players.advance(1);
	else if (nav == input::NAV_UP) m_players.advance(-1);
	else if (nav == input::NAV_DOWN) m_players.advance(1);
	else if (nav == input::NAV_MOREUP) m_players.advance(-10);
	else if (nav == input::NAV_MOREDOWN) m_players.advance(10);
}

void ScreenPlayers::manageEvent(SDL_Event event) {
	keyPressed = true;
	if (event.type == SDL_TEXTINPUT) {
		m_search += event.text.text;
		m_players.setFilter(m_search.text);
	}
	else if (event.type == SDL_KEYDOWN) {
		int key = event.key.keysym.scancode;
		if (key == SDL_SCANCODE_BACKSPACE) {
			m_search.backspace();
			m_players.setFilter(m_search.text);
		}
	}
}

Texture* ScreenPlayers::loadTextureFromMap(fs::path path) {
	if(m_covers.find(path) == m_covers.end()) {
		std::pair<fs::path, std::unique_ptr<Texture>> kv = std::make_pair(path, std::make_unique<Texture>(path));
		m_covers.insert(std::move(kv));
	}
	try {
		return m_covers.at(path).get();
	} catch (std::exception const&) {}
	return nullptr;
}

void ScreenPlayers::draw() {
	m_players.update(); // Poll for new players
	double length = m_audio.getLength();
	double time = clamp(m_audio.getPosition() - config["audio/video_delay"].f(), 0.0, length);
	if (m_songbg.get()) m_songbg->draw();
	if (m_video.get()) m_video->render(time);
	theme->bg.draw();
	std::string music, songbg, video;
	double videoGap = 0.0;
	std::ostringstream oss_song, oss_order;
	// Test if there are no players currently selected
	if (m_players.empty()) {
		// Format the song information text
		if (m_search.text.empty()) {
			oss_song << _("No players found!");
			oss_order << _("Enter a name to create a new player.");
		} else {
			oss_song << _("Press enter to create player!");
			oss_order << m_search.text << '\n';
		}
	} else if (m_database.scores.empty()) {
		oss_song << _("No players worth mentioning!");
	} else {
		// Format the player information text
		oss_song << m_database.scores.front().track << '\n';
		// TODO: use boost::format
		oss_song << boost::format(_("You reached %1% points!")) % m_database.scores.front().score;
		oss_order << _("Change player with arrow keys.") << '\n'
			<< _("Name:") << ' ' << m_players.current().name << '\n';
		//m_database.queryPerPlayerHiscore(oss_order);
		oss_order << '\n'
			<< (m_search.text.empty() ? _("Type text to filter or create a new player.") : std::string(_("Search Text:")) + " " + m_search.text)
			<< '\n';
		double spos = m_players.currentPosition(); // This needs to be polled to run the animation

		// Draw the covers
		std::size_t ss = m_players.size();
		int baseidx = spos + 1.5; --baseidx; // Round correctly
		double shift = spos - baseidx;
		// FIXME: 3D browser
		for (int i = -2; i < 5; ++i) {
			PlayerItem player_display = m_players[baseidx + i];
			if (baseidx + i < 0 || baseidx + i >= int(ss)) continue;
			
			Texture& s = !player_display.path.empty() ? *loadTextureFromMap(player_display.path) : *m_emptyCover;
			double diff = (i == 0 ? (0.5 - fabs(shift)) * 0.07 : 0.0);
			double y = 0.27 + 0.5 * diff;
			// Draw the cover
			s.dimensions.middle(-0.2 + 0.17 * (i - shift)).bottom(y - 0.2 * diff).fitInside(0.14 + diff, 0.14 + diff); s.draw();
			// Draw the reflection
			s.dimensions.top(y + 0.2 * diff); s.tex = TexCoords(0, 1, 1, 0);
			{
				ColorTrans c(Color::alpha(0.4));
				s.draw();
			}
			s.tex = TexCoords();
		}
		/*
		if (!song.music.empty()) music = song.music[0]; // FIXME: support multiple tracks
		if (!song.background.empty()) songbg = song.path + song.background;
		if (!song.video.empty()) { video = song.path + song.video; videoGap = song.videoGap; }
		*/
	}

	// Draw song and order texts
	theme->song.draw(oss_song.str());
	theme->order.draw(oss_order.str());

	// Schedule playback change if the chosen song has changed
	if (music != m_playReq) { m_playReq = music; m_playTimer.setValue(0.0); }
	if (m_quitTimer.get() == 0.0 && !keyPressed) { Game::getSingletonPtr()->activateScreen("Playlist"); return; }
	// Play/stop preview playback (if it is the time)
	if (music != m_playing && m_playTimer.get() > 0.4) {
		m_songbg.reset(); m_video.reset();
		if (music.empty()) m_audio.fadeout(1.0); else m_audio.playMusic(music, true, 2.0);
		if (!songbg.empty()) try { m_songbg = std::make_unique<Texture>(songbg); } catch (std::exception const&) {}
		if (!video.empty() && config["graphic/video"].b()) m_video = std::make_unique<Video>(video, videoGap);
		m_playing = music;
	}
	m_layout_singer->drawScore(LayoutSinger::TOP);
}

