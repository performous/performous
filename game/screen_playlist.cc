#include "screen_playlist.hh"
#include "menu.hh"
#include "screen_songs.hh"
#include "screen_sing.hh"
#include "playlist.hh"
#include "theme.hh"
#include "util.hh"
#include "i18n.hh"

#include <iostream>
#include <sstream>
#include <boost/format.hpp>

namespace {
    static const double NEXT_TIMEOUT = 15.0; // go to next song in 15 seconds
}

screenPlaylist::screenPlaylist(std::string const& name,Audio& audio, Songs& songs):
    Screen(name) ,m_audio(audio), m_songs(songs), m_covers(20), m_playlist() {
}

void screenPlaylist::enter() {
    m_audio.togglePause();
    updatePlayList();
    if(m_playlist.isEmpty())
      {
        ScreenManager* sm = ScreenManager::getSingletonPtr();
        sm->activateScreen("Songs");
      }
    keyPressed = false;
    m_nextTimer.setValue(NEXT_TIMEOUT);
    m_menu.close();
    reloadGL();
    //needs to start timer for 10 seconds here but boost::timer documentation is horrible!
}

void screenPlaylist::prepare() {
    return;
}

void screenPlaylist::reloadGL() {
    theme.reset(new ThemeSongs());
    m_menuTheme.reset(new ThemeInstrumentMenu());
}

void screenPlaylist::exit() {
    m_menu.clear();
    m_menuTheme.reset();
    theme.reset();
    m_audio.togglePause();
}


void screenPlaylist::updatePlayList() {
    ScreenManager* sm = ScreenManager::getSingletonPtr();
    Screen* s = sm->getScreen("Songs");
    ScreenSongs* ss = dynamic_cast<ScreenSongs*> (s);
    assert(ss);
    m_playlist = ss->getPlaylist();
}

void screenPlaylist::manageEvent(input::NavEvent const& event) {
    ScreenManager* sm = ScreenManager::getSingletonPtr();
    input::NavButton nav = event.button;
    if(keyPressed == false) {
    keyPressed = true;
    createEditMenu();
    m_menu.open();
    }
    else if (nav == input::NAV_PAUSE) m_audio.togglePause();
    else if (nav == input::NAV_START) {
        if (m_menu.isOpen()) {
              m_menu.action();
        }
    }
    else if (event.menu == input::NAVMENU_A_PREV) {
        if(m_menu.isOpen()) {
          m_menu.move(-1);
        }
        ///will be for cycling through queued songs later
        //else {
        //  m_songs.advance(-1);
        //}
     }
     else if (event.menu == input::NAVMENU_A_NEXT) {
        if(m_menu.isOpen()) {
          m_menu.move(1);
        }
        ///will be for cycling through queued songs later
        //else {
        //  m_songs.advance(1);
        //}
     }
     else if (event.menu == input::NAVMENU_B_PREV) {
        if(m_menu.isOpen()) {
          m_menu.move(-1);
        }

     }
     else if (event.menu == input::NAVMENU_B_NEXT) {
        if(m_menu.isOpen()) {
          m_menu.move(1);
        }
     }
}

void screenPlaylist::manageEvent(SDL_Event) {
    return;
}

void screenPlaylist::draw()
{
    if (m_nextTimer.get() == 0.0 && keyPressed == false)
    {
        ScreenManager* sm = ScreenManager::getSingletonPtr();
        Screen* s = sm->getScreen("Sing");
        ScreenSing* ss = dynamic_cast<ScreenSing*> (s);
        assert(ss);
        ss->setSong(m_playlist.getNext());
        sm->activateScreen("Sing");
    }
    if (keyPressed == true && !m_menu.isOpen()) m_menu.open();
    drawMenu();
}

void screenPlaylist::createEditMenu() {
 m_menu.clear();
 m_menu.add(MenuOption(_("Continue"), _("Continue playing")).call([this]() {
     ScreenManager* sm = ScreenManager::getSingletonPtr();
     Screen* s = sm->getScreen("Sing");
     ScreenSing* ss = dynamic_cast<ScreenSing*> (s);
     s = sm->getScreen("Songs");
     ScreenSongs* sc = dynamic_cast<ScreenSongs*> (s);
     assert(ss);
     ss->setSong(sc->getPlaylist().getNext());
     sm->activateScreen("Sing");
 }));
 m_menu.add(MenuOption(_("Add songs"), _("Open the song browser to add more songs")).screen("Songs"));
 
 /// will be uncommented when drawing stuff works
 //m_menu.add(MenuOption(_("Remove song"), _("Remove currently selected song from list")).call([this]() {

 //}));
 m_menu.add(MenuOption(_("Quit"), _("Remove all the songs from the list and go back to main menu")).call([this]() {
     ScreenManager* sm = ScreenManager::getSingletonPtr();
     Screen* s = sm->getScreen("Songs");
     ScreenSongs* ss = dynamic_cast<ScreenSongs*> (s);
     ss->getPlaylist().clear();
     sm->activateScreen("Intro");
 }));
}

void screenPlaylist::drawMenu() {
    if (m_menu.empty()) return;
    // Some helper vars
    ThemeInstrumentMenu& th = *m_menuTheme;
    MenuOptions::const_iterator cur = static_cast<MenuOptions::const_iterator>(&m_menu.current());
    double w = m_menu.dimensions.w();
    const float txth = th.option_selected.h();
    const float step = txth * 0.85f;
    const float h = m_menu.getOptions().size() * step + step;
    float y = -h * .5f + step;
    float x = -w * .5f + step;
    // Background
    th.bg.dimensions.middle(0).center(0).stretch(w, h);
    th.bg.draw();
    // Loop through menu items
    w = 0;
    for (MenuOptions::const_iterator it = m_menu.begin(); it != m_menu.end(); ++it) {
        // Pick the font object
        SvgTxtTheme* txt = &th.option_selected;
        if (cur != it) txt = &(th.getCachedOption(it->getName()));
        // Set dimensions and draw
        txt->dimensions.middle(x).center(y);
        txt->draw(it->getName());
        w = std::max(w, txt->w() + 2 * step); // Calculate the widest entry
        y += step;
    }
    if (cur->getComment() != "") {
        th.comment.dimensions.middle(0).screenBottom(-0.12);
        th.comment.draw(cur->getComment());
    }
    m_menu.dimensions.stretch(w, h);
}
