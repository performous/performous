#pragma once

#include "backgrounds.hh"
#include "screen.hh"
#include "menu.hh"
#include "song.hh"
#include "animvalue.hh"
#include "playlist.hh"
#include "controllers.hh"
#include "songs.hh"
#include "graphic/texture.hh"
#include "webcam.hh"
#include "theme/theme.hh"
#include "event_manager.hh"
#include "configuration.hh"

#include <mutex>
#include <vector>
#include <unordered_map>

class Audio;
class Database;
class Song;
class Texture;
class ThemePlaylistScreen;
class Backgrounds;
class ThemeInstrumentMenu;

class ScreenPlaylist : public Screen
{
public:
	typedef std::vector< std::shared_ptr<Song> > SongList;
	ScreenPlaylist(Game &game, std::string const& name, Audio& audio, Songs& songs, Backgrounds& bgs);
	void manageEvent(input::NavEvent const& event);
	void manageEvent(SDL_Event);
	void prepare();
	void draw();
	void enter();
	void exit();
	void reloadGL();
	void triggerSongListUpdate();
private:
	Menu overlay_menu;
	Menu songlist_menu;
	AnimValue m_selAnim;
	AnimValue m_submenuAnim;
	Audio& m_audio;
	Songs& m_songs;
	void createSongListMenu();
	void createEscMenu();
	void createSongMenu(unsigned songNumber);
	void drawMenu();
	void createMenuFromPlaylist();
	Texture* loadTextureFromMap(fs::path path);
	void onEnter(EventParameter const& parameter);

	Backgrounds& m_backgrounds;
	std::unordered_map<fs::path, std::unique_ptr<Texture>, FsPathHash> m_covers;
	std::unique_ptr<ThemeInstrumentMenu> m_menuTheme;
	std::shared_ptr<ThemePlaylistScreen> m_theme;
	std::shared_ptr<Texture> m_background;
	SvgTxtTheme& getTextObject(std::string const& txt);
	AnimValue m_nextTimer;
	void draw_menu_options();
	bool keyPressed;
	bool needsUpdate = false;
	mutable std::mutex m_mutex;
	Texture& getCover(Song const& song);
	std::unique_ptr<Texture> m_singCover;
	std::unique_ptr<Texture> m_instrumentCover;
	std::unique_ptr<Texture> m_bandCover;
	std::unique_ptr<Texture> m_danceCover;
	std::unique_ptr<Webcam> m_cam;
};


