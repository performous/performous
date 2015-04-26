#include "screen_playlist.hh"
#include "menu.hh"
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
	Screen(name), m_audio(audio), m_songs(songs), m_backgrounds(bgs), m_covers(20), keyPressed()
{}

void ScreenPlaylist::enter() {
	Game* gm = Game::getSingletonPtr();
	m_audio.togglePause();
	if (gm->getCurrentPlayList().isEmpty()) {
		if(config["game/autoplay"].b()) {
			m_songs.setFilter("");
			Screen* s = gm->getScreen("Sing");
			ScreenSing* ss = dynamic_cast<ScreenSing*> (s);
			assert(ss);
			int randomsong = std::rand() % m_songs.size();
			ss->setSong(m_songs[randomsong]);
			gm->activateScreen("Sing");
		} else {
			gm->activateScreen("Songs");
		}
	}
	keyPressed = false;
	m_nextTimer.setValue(config["game/playlist_screen_timeout"].i());
	overlay_menu.close();
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
	m_singCover.reset(new Surface(findFile("no_cover.svg")));
	m_instrumentCover.reset(new Surface(findFile("instrument_cover.svg")));
	m_bandCover.reset(new Surface(findFile("band_cover.svg")));
	m_danceCover.reset(new Surface(findFile("dance_cover.svg")));
}

void ScreenPlaylist::exit() {
	overlay_menu.clear();
	songlist_menu.clear();
	m_menuTheme.reset();
	theme.reset();
	m_audio.togglePause();
	m_background.reset();
}


void ScreenPlaylist::manageEvent(input::NavEvent const& event) {
	input::NavButton nav = event.button;
	Menu& menu = overlay_menu.isOpen() ? overlay_menu : songlist_menu;

	if (keyPressed == false)
		keyPressed = true;

	if (nav == input::NAV_CANCEL) {
		if(overlay_menu.isOpen()) {
			overlay_menu.close();
		} else {
			createEscMenu();
			overlay_menu.open();
		}
	} else {
		if (nav == input::NAV_PAUSE) {
			m_audio.togglePause();
		} else if (nav == input::NAV_START) {
			menu.action();
		} else if (event.menu == input::NAVMENU_A_PREV) {
			menu.move(-1);
		} else if (event.menu == input::NAVMENU_A_NEXT) {
			menu.move(1);
		} else if (event.menu == input::NAVMENU_B_PREV) {
			menu.move(-1);
		} else if (event.menu == input::NAVMENU_B_NEXT) {
			menu.move(1);
		}
	}
}

void ScreenPlaylist::manageEvent(SDL_Event) {
	return;
}

void ScreenPlaylist::draw() {
	Game* gm = Game::getSingletonPtr();
	if (!m_background || m_background->empty()) m_background.reset(new Surface(m_backgrounds.getRandom()));
	m_background->draw();
	if (m_nextTimer.get() == 0.0 && keyPressed == false) {
		Screen* s = gm->getScreen("Sing");
		ScreenSing* ss = dynamic_cast<ScreenSing*> (s);
		assert(ss);
		ss->setSong(gm->getCurrentPlayList().getNext());
		gm->activateScreen("Sing");
	}
	draw_menu_options();
	//menu on top of everything
	if (overlay_menu.isOpen()) {
		drawMenu();
	}

	if(needsUpdate) {
	boost::mutex::scoped_lock l(m_mutex);
	createSongListMenu();
	needsUpdate = false;
	}
	auto const& playlist = gm->getCurrentPlayList().getList();
	for (unsigned i = playlist.size() - 1; i < playlist.size(); --i) {
		if(i < 9) { //only draw the first 9 covers
			Surface& s = getCover(*playlist[i]);
			double pos =  i / std::max<double>(9, 9);
			using namespace glmath;
			Transform trans(
			  translate(vec3(-0.4 + 0.9 * pos, 0.045)) //vec3(horizontal-offset-from-center, vertical offset from screen_bottom)
			  * rotate(-0.0, vec3(0.0, 1.0, 0.0))
			);
			s.dimensions.middle().screenBottom(-0.06).fitInside(0.08, 0.08);
			s.draw();
		}
	}
}
Surface& ScreenPlaylist::getCover(Song const& song) {
	Surface* cover = nullptr;
	// Fetch cover image from cache or try loading it
	if (!song.cover.empty()) try { cover = &m_covers[song.cover]; } catch (std::exception const&) {}
	// Fallback to background image as cover if needed
	if (!cover && !song.background.empty()) try { cover = &m_covers[song.background]; } catch (std::exception const&) {}
	// Use empty cover
	if (!cover) {
		if(song.hasDance()) {
			cover = m_danceCover.get();
		} else if(song.hasDrums()) {
			cover = m_bandCover.get();
		} else {
			size_t tracks = song.instrumentTracks.size();
			if (tracks == 0) cover = m_singCover.get();
			else cover = m_instrumentCover.get();
		}
	}
	return *cover;

}

void ScreenPlaylist::createEscMenu() {
	overlay_menu.clear();
	overlay_menu.add(MenuOption(_("Continue"), _("Continue playing")).call([this]() {
		Game* gm = Game::getSingletonPtr();
		Screen* s = gm->getScreen("Sing");
		ScreenSing* ss = dynamic_cast<ScreenSing*> (s);
		assert(ss);
		ss->setSong(gm->getCurrentPlayList().getNext());
		gm->activateScreen("Sing");
	}));
	overlay_menu.add(MenuOption(_("Add songs"), _("Open the song browser to add more songs")).screen("Songs"));
	overlay_menu.add(MenuOption(_("Shuffle"), _("Randomize the order of the playlist")).call([this]() {
		Game* tm = Game::getSingletonPtr();
		tm->getCurrentPlayList().shuffle();
		overlay_menu.close();
		createSongListMenu();
	}));
	overlay_menu.add(MenuOption(_("Clear and exit"), _("Remove all the songs from the list")).call([this]() {
		Game* tm = Game::getSingletonPtr();
		tm->getCurrentPlayList().clear();
		overlay_menu.close();
		tm->activateScreen("Songs");
	}));
	overlay_menu.add(MenuOption(_("Back"), _("Back to playlist viewer")).call([this]() {
		overlay_menu.close();
	}));
}

void ScreenPlaylist::drawMenu() {
	if (overlay_menu.empty()) return;
	// Some helper vars
	ThemeInstrumentMenu& th = *m_menuTheme;
	const auto cur = &overlay_menu.current();
	double w = overlay_menu.dimensions.w();
	const float txth = th.option_selected.h();
	const float step = txth * 0.85f;
	const float h = overlay_menu.getOptions().size() * step + step;
	float y = -h * .5f + step;
	float x = -w * .5f + step;
	// Background
	th.bg.dimensions.middle(0).center(0).stretch(w, h);
	th.bg.draw();
	// Loop through menu items
	w = 0;
	for (MenuOptions::const_iterator it = overlay_menu.begin(); it != overlay_menu.end(); ++it) {
		// Pick the font object
		SvgTxtTheme* txt = &th.option_selected;
		if (cur != &*it)
			txt = &(th.getCachedOption(it->getName()));
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
	overlay_menu.dimensions.stretch(w, h);
}

void ScreenPlaylist::draw_menu_options() {
	// Variables used for positioning and other stuff
	double wcounter = 0;
	const size_t showopts = 7; // Show at most 8 options simultaneously
	const float x = -0.35; // x xcoordinate from screen center, the menu should be aligned left of the center therefore it´s negative.n
	const float start_y = -0.15;
	const float sel_margin = 0.04;
	const MenuOptions opts = songlist_menu.getOptions();
	double submenuanim = 1.0 - std::min(1.0, std::abs(m_submenuAnim.get()-songlist_menu.getSubmenuLevel()));
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
			// Draw the text, dim if option not available
			{
				ColorTrans c(Color::alpha(opt.isActive() ? 1.0 : 0.5));
				theme->option_selected.dimensions.left(x).center(start_y + ii*0.049);
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
			txt.dimensions.left(x).center(start_y + ii*0.05);
			txt.draw(title);
			wcounter = std::max(wcounter, txt.w() + 2 * sel_margin); // Calculate the widest entry
		}
	}
	songlist_menu.dimensions.stretch(wcounter, 1);
}

SvgTxtTheme& ScreenPlaylist::getTextObject(std::string const& txt) {
	if (theme->options.contains(txt)) return theme->options[txt];
	return *theme->options.insert(txt, new SvgTxtTheme(findFile("mainmenu_option.svg"), config["graphic/text_lod"].f()))->second;
}

void ScreenPlaylist::createSongListMenu() {
	Game* gm = Game::getSingletonPtr();
	std::ostringstream oss_playlist;
	int count = 1;
	songlist_menu.clear();
	SongList& currentList = gm->getCurrentPlayList().getList();
	for (auto const& song: currentList) {
		oss_playlist << "#" << count << " : " << song->artist << " - " << song->title;
		std::string songinfo = oss_playlist.str();
		if (songinfo.length() > 20) {
			songinfo = songinfo + "                          >"; //FIXME: ugly hack to make the text scale so it fits on screen!
		}
		songlist_menu.add(MenuOption(_(songinfo.c_str()),_("Press enter to view song options")).call([this, count]() {
			createSongMenu(count);
			overlay_menu.open();
		}));
		oss_playlist.str("");
		count++;
	}
	songlist_menu.add(MenuOption(_("View more options"),_("View general playlist settings")).call([this]() {
		createEscMenu();
		overlay_menu.open();
	}));
}

void ScreenPlaylist::createSongMenu(int songNumber) {
	overlay_menu.clear();
	std::string firstOption = songNumber >= 2 ? _("Play first") : _("Continue");
	std::string firstDesc = songNumber >= 2 ?
		_("Ignore the playlist's order and play this song first") :
		_("Start the song already!");
	overlay_menu.add(MenuOption(firstOption, firstDesc).call([this, songNumber]() {
		Game* gm = Game::getSingletonPtr();
		Screen* s = gm->getScreen("Sing");
		ScreenSing* ss = dynamic_cast<ScreenSing*>(s);
		assert(ss);
		ss->setSong(gm->getCurrentPlayList().getSong(songNumber - 1));
		gm->activateScreen("Sing");
	}));
	overlay_menu.add(MenuOption(_("Remove"), _("Remove this song from the list")).call([this, songNumber]() {
		Game* gm = Game::getSingletonPtr();
		// Minus 1 so it doesn´t remove #2 when you´ve selected #1
		gm->getCurrentPlayList().removeSong(songNumber - 1);
		overlay_menu.close();
		if (gm->getCurrentPlayList().isEmpty()) {
			gm->activateScreen("Songs");
		} else {
			createSongListMenu();
		}
	}));
	if (songNumber >= 2) { //can't move up first song
		overlay_menu.add(MenuOption(_("Move up"), _("Move this song up the list")).call([this, songNumber]() {
			Game* gm = Game::getSingletonPtr();
			gm->getCurrentPlayList().swap(songNumber -1, songNumber -2);
			createSongListMenu();
			overlay_menu.close();
		}));
	}
	Game* gm = Game::getSingletonPtr();
	int size = gm->getCurrentPlayList().getList().size();
	if (songNumber < size) { //can't move down the last song
		overlay_menu.add(MenuOption(_("Move Down"), _("Move this song down the list")).call([this, songNumber]() {
			Game* gm = Game::getSingletonPtr();
			gm->getCurrentPlayList().swap(songNumber -1, songNumber);
			createSongListMenu();
			overlay_menu.close();
		}));
	}
	overlay_menu.add(MenuOption(_("Back"), _("Back to playlist viewer")).call([this]() {
		overlay_menu.close();
	}));
}

void ScreenPlaylist::triggerSongListUpdate() {
boost::mutex::scoped_lock l (m_mutex);
needsUpdate = true;
}
