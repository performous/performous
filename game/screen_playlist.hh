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
class ThemeSongs;
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
  Menu m_menu;
  Audio& m_audio;
  Songs& m_songs;
  void updatePlayList();
  void createEditMenu();
  void drawMenu();
  PlayList m_playlist;
  Backgrounds& m_backgrounds;
  Cachemap<std::string, Surface> m_covers;
  boost::scoped_ptr<ThemeInstrumentMenu> m_menuTheme;
  boost::scoped_ptr<ThemeSongs> theme;
  boost::scoped_ptr<Surface> m_background;
  AnimValue m_nextTimer;
};


