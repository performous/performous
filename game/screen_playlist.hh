#pragma once

#include "screen.hh"
#include "cachemap.hh"
#include "menu.hh"
#include "song.hh"
#include "animvalue.hh"
#include "playlist.hh"
#include "controllers.hh"

#include <boost/scoped_ptr.hpp>

class Audio;
class Database;
class Song;
class Songs;
class Surface;
class ThemeSongs;
class Backgrounds;
class ThemeInstrumentMenu;


class screen_playlist : public Screen
{
public:
  screen_playlist(std::string const& name,Audio& audio, Songs& songs);
  void manageEvent(input::NavEvent const& event);
  void manageEvent(SDL_Event) = 0;
  void prepare() = 0;
  void draw() = 0;
  void enter();
  void exit();
  void reloadGL();
private:
  Menu m_menu;
  Audio& m_audio;
  Songs& m_songs;
  void updatePlayList();
  void createEditMenu();
  PlayList m_playlist;
  Cachemap<std::string, Surface> m_covers;
  boost::scoped_ptr<ThemeInstrumentMenu> m_menuTheme;
  boost::scoped_ptr<ThemeSongs> theme;
};


