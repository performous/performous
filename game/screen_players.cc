#include "screen_players.hh"
#include "screen_songs.hh"

#include "game.hh"
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

#include <fmt/format.h>

#include <iostream>

ScreenPlayers::ScreenPlayers(Game &game, std::string const& name, Audio& audio, Database& database):
  Screen(game, name), m_audio(audio), m_database(database), m_players(database.m_players)
{
	m_players.setAnimMargins(5.0, 5.0);
	m_playTimer.setTarget(getInf()); // Using this as a simple timer counting seconds
}

void ScreenPlayers::enter() {
	keyPressed = false;
	const auto scaler = NoteGraphScalerFactory(config).create(m_song->getVocalTrack(0u));
	m_layout_singer = std::make_unique<LayoutSinger>(m_song->getVocalTrack(0u), m_database, scaler);
	theme = std::make_unique<ThemeSongs>();
	m_emptyCover = std::make_unique<Texture>(findFile("no_player_image.svg"));
	m_search.text.clear();
	m_players.setFilter(m_search.text);
	m_audio.fadeout(getGame());
	m_quitTimer.setValue(config["game/highscore_timeout"].ui());
	if (m_database.scores.empty() || !m_database.reachedHiscore(m_song)) {
		getGame().activateScreen("Playlist");
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
	input::NavButton nav = event.button;
	if (nav == input::NavButton::CANCEL) {
		if (m_search.text.empty()) { getGame().activateScreen("Songs"); return; }
		else { m_search.text.clear(); m_players.setFilter(m_search.text); }
	} else if (nav == input::NavButton::START) {
		if (m_players.isEmpty()) {
			m_players.addPlayer(m_search.text);
			m_players.setFilter(m_search.text);
			m_players.update();
			// the current player is the new created one
		}
		m_database.addHiscore(m_song);
		m_database.scores.pop_front();

		if (m_database.scores.empty() || !m_database.reachedHiscore(m_song)) {
			// no more highscore, we are now finished
			getGame().activateScreen("Playlist");
		} else {
			m_search.text.clear();
			m_players.setFilter("");
			// add all players which reach highscore because if score is very near or same it might be
			// frustrating for second one that he cannot enter, so better go for next one...
		}
	}
	else if (m_players.isEmpty()) return;
	else if (nav == input::NavButton::PAUSE) m_audio.togglePause();
	else if (nav == input::NavButton::LEFT) m_players.advance(-1);
	else if (nav == input::NavButton::RIGHT) m_players.advance(1);
	else if (nav == input::NavButton::UP) m_players.advance(-1);
	else if (nav == input::NavButton::DOWN) m_players.advance(1);
	else if (nav == input::NavButton::MOREUP) m_players.advance(-10);
	else if (nav == input::NavButton::MOREDOWN) m_players.advance(10);
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
		m_covers.insert({ path, std::make_unique<Texture>(path) });
	}
	try {
		return m_covers.at(path).get();
	} catch (std::exception const&) {}
	return nullptr;
}

void ScreenPlayers::draw() {
	auto& window = getGame().getWindow();
	m_players.update(); // Poll for new players
	double length = m_audio.getLength();
	double time = clamp(m_audio.getPosition() - config["audio/video_delay"].f(), 0.0, length);
	if (m_songbg.get()) m_songbg->draw(window);
	if (m_video.get()) m_video->render(window, time);
	theme->bg.draw(window);
	std::string music, songbg, video;
	double videoGap = 0.0;
	std::string oss_song, oss_order;
	// Test if there are no players currently selected
	if (m_players.isEmpty()) {
		// Format the song information text
		if (m_search.text.empty()) {
			oss_song = _("No Players found!");
			oss_order = _("Enter a name to create a new player.");
		} else {
			oss_song = _("Press enter to create player!");
			oss_order = fmt::format("{}\n", m_search.text);
		}
	} else if (m_database.scores.empty()) {
		oss_song = _("No players worth mentioning!");
	} else {
		// Format the player information text

		oss_song =  fmt::format("{}\n{}", m_database.scores.front().track, fmt::format(_("You reached {0} points!"), m_database.scores.front().score));
		oss_order = fmt::format("{}\n{} {}\n", _("Change player with arrow keys."), _("Name:"), m_players.current().name);
		//m_database.queryPerPlayerHiscore(oss_order);
		oss_order.append("\n");
		if (m_search.text.empty()) {
			oss_order.append(_("Type text to filter or create a new player."));
		}
		else {
			oss_order.append(fmt::format("{} {}\n", _("Search Text:"), m_search.text));
		}
		double spos = m_players.currentPosition(); // This needs to be polled to run the animation

		// Draw the covers
		const unsigned ss = m_players.count();
		double baseidx = spos + 1.5f; --baseidx; // Round correctly
		double shift = spos - baseidx;
		// FIXME: 3D browser
		for (int i = -2; i < 5; ++i) {
			PlayerItem player_display = m_players[static_cast<unsigned>(baseidx + i)];
			if (static_cast<unsigned>(baseidx + i) >= ss) continue;

			Texture& s = !player_display.path.empty() ? *loadTextureFromMap(player_display.path) : *m_emptyCover;
			float diff = (i == 0 ? static_cast<float>((0.5 - fabs(shift)) * 0.07) : 0.0f);
			float y = 0.27f + 0.5f * diff;
			// Draw the cover
			s.dimensions.middle(static_cast<float>(-0.2f + 0.17f * (i - shift))).bottom(y - 0.2f * diff).fitInside(0.14f + diff, 0.14f + diff); s.draw(window);
			// Draw the reflection
			s.dimensions.top(y + 0.2f * diff); s.tex = TexCoords(0, 1, 1, 0);
			{
				ColorTrans c(window, Color::alpha(0.4f));
				s.draw(window);
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
	theme->song.draw(window, oss_song);
	theme->order.draw(window, oss_order);

	// Schedule playback change if the chosen song has changed
	if (music != m_playReq) { m_playReq = music; m_playTimer.setValue(0.0); }
	if (m_quitTimer.get() == 0.0 && !keyPressed) { getGame().activateScreen("Playlist"); return; }
	// Play/stop preview playback (if it is the time)
	if (music != m_playing && m_playTimer.get() > 0.4) {
		m_songbg.reset(); m_video.reset();
		if (music.empty())
			m_audio.fadeout(getGame(), 1.0f);
		else
			m_audio.playMusic(getGame(), music, true, 2.0);
		if (!songbg.empty()) try { m_songbg = std::make_unique<Texture>(songbg); } catch (std::exception const&) {}
		if (!video.empty() && config["graphic/video"].b()) m_video = std::make_unique<Video>(video, videoGap);
		m_playing = music;
	}
	m_layout_singer->drawScore(window, LayoutSinger::PositionMode::TOP);
}
