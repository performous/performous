#pragma once

#include "backgrounds.hh"
#include "screen.hh"
#include "cachemap.hh"
#include "menu.hh"
#include "song.hh"
#include "animvalue.hh"
#include "playlist.hh"
#include "controllers.hh"
#include <vector>
#include <boost/scoped_ptr.hpp>

class Audio;
class Database;
class Song;
class Songs;
class Surface;
class ThemePlaylistScreen;
class Backgrounds;
class ThemeInstrumentMenu;


class ScreenPlaylist : public Screen
{
public:
  typedef std::vector< boost::shared_ptr<Song> > SongList;
  ScreenPlaylist(std::string const& name,Audio& audio, Songs& songs, Backgrounds& bgs);
  void manageEvent(input::NavEvent const& event);
  void manageEvent(SDL_Event);
  void prepare();
  void draw();
  void enter();
  void exit();
  void reloadGL();
private:
  bool keyPressed = false;
  Menu esc_menu;
  Menu songlist_menu;
  bool m_first;
  AnimValue m_selAnim;
  AnimValue m_submenuAnim;
  Audio& m_audio;
  Songs& m_songs;
  void createSongListMenu();
  void createEscMenu();
  void createSongMenu();
  void drawMenu();
  void createMenuFromPlaylist();
  Backgrounds& m_backgrounds;
  Cachemap<std::string, Surface> m_covers;
  boost::scoped_ptr<ThemeInstrumentMenu> m_menuTheme;
  boost::scoped_ptr<ThemePlaylistScreen> theme;
  boost::scoped_ptr<Surface> m_background;
  SvgTxtTheme& getTextObject(std::string const& txt);
  AnimValue m_nextTimer;
  void draw_menu_options();
};


