#pragma once

#include "animvalue.hh"
#include "controllers.hh"
#include "screen.hh"
#include "theme.hh"
#include "song.hh" // for MusicFiles class
#include "textinput.hh"
#include "video.hh"
#include "playlist.hh"
#include "menu.hh"
#include "aubio/aubio.h"

class Audio;
class Database;
class Song;
class Songs;
class Texture;
class ThemeSongs;

class Backgrounds;
class ThemeInstrumentMenu;

/// song chooser screen
class ScreenSongs : public Screen {
public:
	/// constructor
	ScreenSongs(std::string const& name, Audio& audio, Songs& songs, Database& database);
	void enter();
	void exit();
	void reloadGL();
	void menuBrowse(int dir); ///< Left/Right on menu options
	void manageEvent(SDL_Event event);
	void manageEvent(input::NavEvent const& event);
	Songs& getSongs() const { return m_songs; }
	void prepare();
	void draw();
	void drawCovers(); ///< draw the cover browser
	Texture& getCover(Song const& song); ///< get appropriate cover image for the song (incl. no cover)
	void drawJukebox(); ///< draw the songbrowser in jukebox mode (fullscreen, full previews, ...)
	static std::unique_ptr<fvec_t, void(*)(fvec_t*)> previewBeatsBuffer;
private:
	void manageSharedKey(input::NavEvent const& event); ///< same behaviour for jukebox and normal mode
	void drawInstruments(Dimensions dim) const;
	void drawMultimedia();
	void update();
	void drawMenu();
	bool addSong(); ///< Add current song to playlist. Returns true if the playlist was empty.
	void sing(); ///< Enter singing screen with current playlist.
	void createPlaylistMenu();
	Texture* loadTextureFromMap(fs::path path);

	Audio& m_audio;
	Songs& m_songs;
	Database& m_database;
	std::unique_ptr<Texture> m_songbg, m_songbg_ground, m_songbg_default;
	std::unique_ptr<Video> m_video;
	std::unique_ptr<ThemeSongs> theme;
	Song::MusicFiles m_playing;
	AnimValue m_clock;
	AnimValue m_idleTimer;
	TextInput m_search;
	std::unique_ptr<Texture> m_singCover;
	std::unique_ptr<Texture> m_instrumentCover;
	std::unique_ptr<Texture> m_bandCover;
	std::unique_ptr<Texture> m_danceCover;
	std::unique_ptr<Texture> m_instrumentList;
	std::unique_ptr<ThemeInstrumentMenu> m_menuTheme;
	std::map<fs::path, std::unique_ptr<Texture>> m_covers;
	int m_menuPos, m_infoPos;
	bool m_jukebox;
	Menu m_menu;
};
