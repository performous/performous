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

ScreenPlaylist::ScreenPlaylist(std::string const& name,Audio& audio, Songs& songs, Backgrounds& bgs):
    Screen(name) ,m_audio(audio), m_songs(songs), m_covers(20), m_backgrounds(bgs) {
}

void ScreenPlaylist::enter() {
    Game* gm = Game::getSingletonPtr();
    m_audio.togglePause();
    if(gm->getCurrentPlayList().isEmpty())
      {
        gm->activateScreen("Songs");
      }
    keyPressed = false;
    m_nextTimer.setValue(NEXT_TIMEOUT);
    m_menu.close();
    reloadGL();
}

void ScreenPlaylist::prepare() {
    return;
}

void ScreenPlaylist::reloadGL() {
    theme.reset(new ThemeSongs());
    m_menuTheme.reset(new ThemeInstrumentMenu());
}

void ScreenPlaylist::exit() {
    m_menu.clear();
    m_menuTheme.reset();
    theme.reset();
    m_audio.togglePause();
    m_background.reset();
}


void ScreenPlaylist::manageEvent(input::NavEvent const& event) {
  if(keyPressed == false) {
    keyPressed = true;
    createEditMenu();
    m_menu.open();
    }
  else {
    input::NavButton nav = event.button;
    if (nav == input::NAV_PAUSE) m_audio.togglePause();
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
}

void ScreenPlaylist::manageEvent(SDL_Event) {
    return;
}

void ScreenPlaylist::draw()
{
    Game* gm = Game::getSingletonPtr();
    if (!m_background || m_background->empty()) m_background.reset(new Surface(m_backgrounds.getRandom()));
    m_background->draw();
    theme->song.setAlign(SvgTxtTheme::CENTER);
    theme->song.dimensions.screenCenter().middle();
    theme->song.draw(gm->getCurrentPlayList().getListAsString());
    if (m_nextTimer.get() == 0.0 && keyPressed == false)
    {
        Screen* s = gm->getScreen("Sing");
        ScreenSing* ss = dynamic_cast<ScreenSing*> (s);
        assert(ss);
        ss->setSong(gm->getCurrentPlayList().getNext());
        gm->activateScreen("Sing");
    }
    //menu on top of everything
    if (keyPressed == true && !m_menu.isOpen()) {
        m_menu.open();
        drawMenu();
    }
    if (m_menu.isOpen()) {
        drawMenu();
    }


}

void ScreenPlaylist::createEditMenu() {
 m_menu.clear();
 m_menu.add(MenuOption(_("Continue"), _("Continue playing")).call([this]() {
     Game* gm = Game::getSingletonPtr();
     Screen* s = gm->getScreen("Sing");
     ScreenSing* ss = dynamic_cast<ScreenSing*> (s);
     assert(ss);
     ss->setSong(gm->getCurrentPlayList().getNext());
     gm->activateScreen("Sing");
 }));
 m_menu.add(MenuOption(_("Add songs"), _("Open the song browser to add more songs")).screen("Songs"));
 
 /// will be uncommented when drawing stuff works
 //m_menu.add(MenuOption(_("Remove song"), _("Remove currently selected song from list")).call([this]() {

 //}));
 m_menu.add(MenuOption(_("Quit"), _("Remove all the songs from the list and go back to main menu")).call([this]() {
     Game* gm = Game::getSingletonPtr();
     gm->getCurrentPlayList().clear();
     gm->activateScreen("Intro");
 }));
}

void ScreenPlaylist::drawMenu() {
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
