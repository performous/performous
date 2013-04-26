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
    esc_menu.close();
    createSongListMenu();
    songlist_menu.open();
    reloadGL();
}

void ScreenPlaylist::prepare() {
    return;
}

void ScreenPlaylist::reloadGL() {
    theme.reset(new ThemePlaylistScreen());
    m_menuTheme.reset(new ThemeInstrumentMenu());
}

void ScreenPlaylist::exit() {
    esc_menu.clear();
    songlist_menu.clear();
    m_menuTheme.reset();
    theme.reset();
    m_audio.togglePause();
    m_background.reset();
}


void ScreenPlaylist::manageEvent(input::NavEvent const& event) {
  input::NavButton nav = event.button;
  if(keyPressed == false)
    {
  keyPressed = true;
    }
    if(nav == input::NAV_CANCEL) {
    createEscMenu();
    esc_menu.open();
    }
  else {
    if (nav == input::NAV_PAUSE) m_audio.togglePause();
    else if (nav == input::NAV_START) {
        if (esc_menu.isOpen()) {
          esc_menu.action();
        }
        else {
          songlist_menu.action();
        }

    }
    else if (event.menu == input::NAVMENU_A_PREV) {
        if(esc_menu.isOpen()) {
          esc_menu.move(-1);
        }
        else {
          songlist_menu.move(-1);
        }
     }
     else if (event.menu == input::NAVMENU_A_NEXT) {
        if(esc_menu.isOpen()) {
          esc_menu.move(1);
        }
        else {
          songlist_menu.move(1);
        }
     }
     else if (event.menu == input::NAVMENU_B_PREV) {
        if(esc_menu.isOpen()) {
          esc_menu.move(-1);
        }
        else {
          songlist_menu.move(-1);
        }

     }
     else if (event.menu == input::NAVMENU_B_NEXT) {
        if(esc_menu.isOpen()) {
          esc_menu.move(1);
        }
        else {
          songlist_menu.move(1);
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
    if (m_nextTimer.get() == 0.0 && keyPressed == false)
    {
        Screen* s = gm->getScreen("Sing");
        ScreenSing* ss = dynamic_cast<ScreenSing*> (s);
        assert(ss);
        ss->setSong(gm->getCurrentPlayList().getNext());
        gm->activateScreen("Sing");
    }
    draw_menu_options();
    //menu on top of everything
    if (esc_menu.isOpen()) {
        drawMenu();
    }


}

void ScreenPlaylist::createEscMenu() {
 esc_menu.clear();
 esc_menu.add(MenuOption(_("Continue"), _("Continue playing")).call([this]() {
     Game* gm = Game::getSingletonPtr();
     Screen* s = gm->getScreen("Sing");
     ScreenSing* ss = dynamic_cast<ScreenSing*> (s);
     assert(ss);
     ss->setSong(gm->getCurrentPlayList().getNext());
     gm->activateScreen("Sing");
 }));
 esc_menu.add(MenuOption(_("Add songs"), _("Open the song browser to add more songs")).screen("Songs"));
 
 /// will be uncommented when drawing stuff works
 //m_menu.add(MenuOption(_("Remove song"), _("Remove currently selected song from list")).call([this]() {

 //}));
 esc_menu.add(MenuOption(_("Quit"), _("Remove all the songs from the list and go back to main menu")).call([this]() {
     Game* gm = Game::getSingletonPtr();
     gm->getCurrentPlayList().clear();
     gm->activateScreen("Intro");
 }));
}

void ScreenPlaylist::drawMenu() {
    if (esc_menu.empty()) return;
    // Some helper vars
    ThemeInstrumentMenu& th = *m_menuTheme;
    MenuOptions::const_iterator cur = static_cast<MenuOptions::const_iterator>(&esc_menu.current());
    double w = esc_menu.dimensions.w();
    const float txth = th.option_selected.h();
    const float step = txth * 0.85f;
    const float h = esc_menu.getOptions().size() * step + step;
    float y = -h * .5f + step;
    float x = -w * .5f + step;
    // Background
    th.bg.dimensions.middle(0).center(0).stretch(w, h);
    th.bg.draw();
    // Loop through menu items
    w = 0;
    for (MenuOptions::const_iterator it = esc_menu.begin(); it != esc_menu.end(); ++it) {
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
    esc_menu.dimensions.stretch(w, h);
}
void ScreenPlaylist::draw_menu_options() {
	// Variables used for positioning and other stuff
	double wcounter = 0;
	const size_t showopts = 10; // Show at most 10 options simultaneously
	const float x = -0.35;
	const float start_y = -0.1;
	const float sel_margin = 0.05;
	const MenuOptions opts = songlist_menu.getOptions();
	double submenuanim = 1.0 - std::min(1.0, std::abs(m_submenuAnim.get()-songlist_menu.getSubmenuLevel()));
	theme->back_h.dimensions.fixedHeight(0.08f);
	theme->back_h.dimensions.stretch(songlist_menu.dimensions.w(), theme->back_h.dimensions.h());
	// Determine from which item to start
	int start_i = std::min((int)songlist_menu.curIndex() - 1, (int)opts.size() - (int)showopts
		+ (songlist_menu.getSubmenuLevel() == 2 ? 1 : 0)); // Hack to counter side-effects from displaying the value inside the menu
	if (start_i < 0 || opts.size() == showopts) start_i = 0;

	// Loop the currently visible options
	for (size_t i = start_i, ii = 0; ii < showopts && i < opts.size(); ++i, ++ii) {
		MenuOption const& opt = opts[i];
		ColorTrans c(Color::alpha(submenuanim));

		// Selection
		if (i == songlist_menu.curIndex()) {
			// Animate selection higlight moving
			double selanim = m_selAnim.get() - start_i;
			if (selanim < 0) selanim = 0;
			theme->back_h.dimensions.left(x - sel_margin).center(start_y+0.003 + selanim*0.08);
			theme->back_h.draw();
			// Draw the text, dim if option not available
			{
				ColorTrans c(Color::alpha(opt.isActive() ? 1.0 : 0.5));
				theme->option_selected.dimensions.left(x).center(start_y + ii*0.08);
				theme->option_selected.draw(opt.getName());
			}
			wcounter = std::max(wcounter, theme->option_selected.w() + 2 * sel_margin); // Calculate the widest entry
			// If this is a config item, show the value below
			if (opt.type == MenuOption::CHANGE_VALUE) {
				++ii; // Use a slot for the value
				theme->option_selected.dimensions.left(x + sel_margin).center(-0.1 + (selanim+1)*0.08);
				theme->option_selected.draw("<  " + opt.value->getValue() + "  >");
			}

		// Regular option (not selected)
		} else {
			std::string title = opt.getName();
			SvgTxtTheme& txt = getTextObject(title);
			ColorTrans c(Color::alpha(opt.isActive() ? 1.0 : 0.5));
			txt.dimensions.left(x).center(start_y + ii*0.08);
			txt.draw(title);
			wcounter = std::max(wcounter, txt.w() + 2 * sel_margin); // Calculate the widest entry
		}
	}
	songlist_menu.dimensions.stretch(wcounter, 1);
}
SvgTxtTheme& ScreenPlaylist::getTextObject(std::string const& txt) {
  if (theme->options.contains(txt)) return theme->options[txt];
  return *theme->options.insert(txt, new SvgTxtTheme(getThemePath("mainmenu_option.svg"), config["graphic/text_lod"].f()))->second;
}

void ScreenPlaylist::createSongListMenu() {
  Game* gm = Game::getSingletonPtr();
  std::ostringstream oss_playlist;
  int count = 1;
  songlist_menu.clear();
  SongList& currentList = gm->getCurrentPlayList().getList();
  for(SongList::iterator it = currentList.begin(); it != currentList.end(); ++it)
    {
      boost::shared_ptr<Song> songToAdd =  (*it);
      oss_playlist << "#" << count << " : " << songToAdd->artist << " - " << songToAdd->title;
      songlist_menu.add(MenuOption(_(oss_playlist.str().c_str()),_("Press enter to view song options")).call([]() {

        }));
      oss_playlist.str("");
      count++;
    }
  songlist_menu.add(MenuOption(_("View more options"),_("View general playlist settings")).call([this]() {
      createEscMenu();
      esc_menu.open();
    }));
}
