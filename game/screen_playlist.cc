#include "screen_playlist.hh"
#include "menu.hh"
#include "screen_songs.hh"
#include "playlist.hh"
#include "theme.hh"
#include "util.hh"
#include "i18n.hh"

#include <iostream>
#include <sstream>
#include <boost/format.hpp>

screen_playlist::screen_playlist(std::string const& name,Audio& audio, Songs& songs):
    Screen(name) ,m_audio(audio), m_songs(songs), m_covers(20), m_playlist() {
}

void screen_playlist::enter() {
    m_menu.close();
    updatePlayList();
    reloadGL();
    //needs to start timer for 10 seconds here but boost::timer documentation is horrible!
}

void screen_playlist::reloadGL() {
    theme.reset(new ThemeSongs());
    m_menuTheme.reset(new ThemeInstrumentMenu());
}

void screen_playlist::exit() {
    m_menu.clear();
    m_menuTheme.reset();
    theme.reset();
}


void screen_playlist::updatePlayList() {
    ScreenManager* sm = ScreenManager::getSingletonPtr();
    Screen* s = sm->getScreen("Songs");
    ScreenSongs* ss = dynamic_cast<ScreenSongs*> (s);
    assert(ss);
    m_playlist = ss->getPlaylist();
}

void screen_playlist::manageEvent(input::NavEvent const& event) {
//needs to stop timer here
    createEditMenu();
    m_menu.open();
}

void screen_playlist::createEditMenu() {
 m_menu.add(MenuOption(_("Continue"), _("Continue playing")).call([this]() {

 }));
 m_menu.add(MenuOption(_("Add songs"), _("Open the song browser to add more songs")).call([this]() {

 }));
 m_menu.add(MenuOption(_("Remove song"), _("Remove currently selected song from list")).call([this]() {

 }));
 m_menu.add(MenuOption(_("Quit"), _("Remove all the songs from the list and go back to main menu")).call([this]() {

 }));
}
