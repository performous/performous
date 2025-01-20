#include "screen_playlist.hh"
#include "menu.hh"
#include "screen_sing.hh"
#include "playlist.hh"
#include "theme.hh"
#include "util.hh"
#include "i18n.hh"

#include <iostream>
#include <sstream>

ScreenPlaylist::ScreenPlaylist(Game &game, std::string const& name,Audio& audio, Songs& songs, Backgrounds& bgs):
	Screen(game, name), m_audio(audio), m_songs(songs), m_backgrounds(bgs), keyPressed()
{}

void ScreenPlaylist::enter() {
	// Initialize webcam
	getGame().loading(_("Initializing webcam..."), 0.1f);
	if (config["graphic/webcam"].b() && Webcam::enabled()) {
		try {
			m_cam = std::make_unique<Webcam>(getGame().getWindow(), config["graphic/webcamid"].ui());
		} catch (std::exception& e) {
			SpdLogger::error(LogSystem::WEBCAM, "Exception={}", e.what());
		}
	}
	m_audio.togglePause();
	if (getGame().getCurrentPlayList().isEmpty()) {
		if(!config["game/autoplay"].b()) {
			getGame().activateScreen("Songs");
		}
	}
	keyPressed = false;
	auto timervalue = config["game/playlist_screen_timeout"].ui();
	if(timervalue < 0.0) {
		timervalue = 0.0;
	}
	m_nextTimer.setValue(timervalue);
	overlay_menu.close();
	getGame().loading(_("Loading song timestamps..."), 0.2f);
	createSongListMenu();
	songlist_menu.open();
	reloadGL();
}

void ScreenPlaylist::prepare() {
	return;
}

void ScreenPlaylist::reloadGL() {
	theme = std::make_unique<ThemePlaylistScreen>();
	m_menuTheme = std::make_unique<ThemeInstrumentMenu>();
	m_singCover = std::make_unique<Texture>(findFile("no_cover.svg"));
	m_instrumentCover = std::make_unique<Texture>(findFile("instrument_cover.svg"));
	m_bandCover = std::make_unique<Texture>(findFile("band_cover.svg"));
	m_danceCover = std::make_unique<Texture>(findFile("dance_cover.svg"));
}

void ScreenPlaylist::exit() {
	overlay_menu.clear();
	songlist_menu.clear();
	m_menuTheme.reset();
	theme.reset();
	m_audio.togglePause();
	m_background.reset();
	m_cam.reset();
}


void ScreenPlaylist::manageEvent(input::NavEvent const& event) {
	input::NavButton nav = event.button;
	Menu& menu = overlay_menu.isOpen() ? overlay_menu : songlist_menu;

	if (keyPressed == false)
		keyPressed = true;

	if (nav == input::NavButton::CANCEL) {
		if(overlay_menu.isOpen()) {
			overlay_menu.close();
		} else {
			createEscMenu();
			overlay_menu.open();
		}
	} else {
		if (nav == input::NavButton::PAUSE) {
			m_audio.togglePause();
		} else if (nav == input::NavButton::START) {
			menu.action(getGame());
		} else if (event.menu == input::NavMenu::A_PREV) {
			menu.move(-1);
		} else if (event.menu == input::NavMenu::A_NEXT) {
			menu.move(1);
		} else if (event.menu == input::NavMenu::B_PREV) {
			menu.move(-1);
		} else if (event.menu == input::NavMenu::B_NEXT) {
			menu.move(1);
		}
	}
}

void ScreenPlaylist::manageEvent(SDL_Event) {
	return;
}

void ScreenPlaylist::draw() {
	auto& window = getGame().getWindow();
	if (!m_background || m_background->empty()) m_background = std::make_unique<Texture>(m_backgrounds.getRandom());
	m_background->draw(window);
	if (m_nextTimer.get() == 0.0 && keyPressed == false) {
		Screen* s = getGame().getScreen("Sing");
		ScreenSing* ss = dynamic_cast<ScreenSing*> (s);
		assert(ss);
		if(getGame().getCurrentPlayList().isEmpty()) {
			m_songs.setFilter("");
			unsigned randomsong = static_cast<unsigned>(std::rand()) % static_cast<unsigned>(m_songs.size());
			ss->setSong(m_songs[randomsong]);
		} else {
			ss->setSong(getGame().getCurrentPlayList().getNext());
		}
		getGame().activateScreen("Sing");
	}
	if (m_cam && config["graphic/webcam"].b()) m_cam->render();
	draw_menu_options();
	//menu on top of everything
	if (overlay_menu.isOpen()) {
		drawMenu();
	}
	if(needsUpdate) {
		std::lock_guard<std::mutex> l(m_mutex);
		createSongListMenu();
		needsUpdate = false;
	}
	auto const& playlist = getGame().getCurrentPlayList().getList();
	for (size_t i = playlist.size() - 1; i < playlist.size(); --i) {
		if(i < 9) { //only draw the first 9 covers
			Texture& s = getCover(*playlist[i]);
			float pos =  static_cast<float>(i) / std::max<float>(9, 9);
			using namespace glmath;
			Transform trans(window,
			  translate(vec3(-0.4f + 0.9f * pos, 0.045f, 0.0f)) //vec3(horizontal-offset-from-center, vertical offset from screen_bottom)
			  * rotate(-0.0, vec3(0.0f, 1.0f, 0.0f))
			);
			s.dimensions.middle().screenBottom(-0.06f).fitInside(0.08f, 0.08f);
			s.draw(window);
		}
	}
}

Texture* ScreenPlaylist::loadTextureFromMap(fs::path path) {
	if(m_covers.find(path) == m_covers.end()) {
		m_covers.insert({ path, std::make_unique<Texture>(path) });
	}
	try {
		return m_covers.at(path).get();
	} catch (std::exception const&) {}
	return nullptr;
}

Texture& ScreenPlaylist::getCover(Song const& song) {
	Texture* cover = nullptr;
	// Fetch cover image from cache or try loading it
	if (!song.cover.empty()) cover = loadTextureFromMap(song.cover);
	// Fallback to background image as cover if needed
	if (!cover && !song.background.empty()) cover = loadTextureFromMap(song.background);
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
	overlay_menu.add(MenuOption(_("Continue"), _("Continue playing"))).call([this]() {
		Screen* s = getGame().getScreen("Sing");
		ScreenSing* ss = dynamic_cast<ScreenSing*> (s);
		assert(ss);
		if(!getGame().getCurrentPlayList().isEmpty()) {
			ss->setSong(getGame().getCurrentPlayList().getNext());
		} else {
			m_songs.setFilter("");
			unsigned randomsong = static_cast<unsigned>(std::rand()) % static_cast<unsigned>(m_songs.size());
			ss->setSong(m_songs[randomsong]);
		}
		getGame().activateScreen("Sing");
	});
	overlay_menu.add(MenuOption(_("Add songs"), _("Open the song browser to add more songs"))).screen("Songs");
	overlay_menu.add(MenuOption(_("Shuffle"), _("Randomize the order of the playlist"))).call([this]() {
		getGame().getCurrentPlayList().shuffle();
		overlay_menu.close();
		createSongListMenu();
	});
	overlay_menu.add(MenuOption(_("Clear and exit"), _("Remove all the songs from the list"))).call([this]() {
		getGame().getCurrentPlayList().clear();
		overlay_menu.close();
		getGame().activateScreen("Songs");
	});
	overlay_menu.add(MenuOption(_("Back"), _("Back to playlist viewer"))).call([this]() {
		overlay_menu.close();
	});
}

void ScreenPlaylist::drawMenu() {
	if (overlay_menu.empty()) return;
	auto& window = getGame().getWindow();
	// Some helper vars
	ThemeInstrumentMenu& th = *m_menuTheme;
	const auto cur = &overlay_menu.current();
	float w = overlay_menu.dimensions.w();
	const float txth = th.option_selected.h();
	const float step = txth * 0.85f;
	const float h = static_cast<float>(overlay_menu.getOptions().size()) * step + step;
	float y = -h * .5f + step;
	float x = -w * .5f + step;
	// Background
	th.bg.dimensions.middle(0).center(0).stretch(w, h);
	th.bg.draw(window);
	// Loop through menu items
	w = 0;
	for (MenuOptions::const_iterator it = overlay_menu.begin(); it != overlay_menu.end(); ++it) {
		// Pick the font object
		SvgTxtTheme* txt = &th.option_selected;
		if (cur != &*it)
			txt = &(th.getCachedOption(it->getName()));
		// Set dimensions and draw
		txt->dimensions.middle(x).center(y);
		txt->draw(window, it->getName());
		w = std::max(w, txt->w() + 2 * step); // Calculate the widest entry
		y += step;
	}
	if (cur->getComment() != "") {
		th.comment.dimensions.middle(0).screenBottom(-0.12f);
		th.comment.draw(window, cur->getComment());
	}
	overlay_menu.dimensions.stretch(w, h);
}

void ScreenPlaylist::draw_menu_options() {
	auto& window = getGame().getWindow();
	// Variables used for positioning and other stuff
	float wcounter = 0.0f;
	const unsigned showopts = 7; // Show at most 8 options simultaneously
	const float x = -0.35f; // x xcoordinate from screen center, the menu should be aligned left of the center therefore it´s negative.n
	const float start_y = -0.15f;
	const float sel_margin = 0.04f;
	const MenuOptions &opts = songlist_menu.getOptions();
	double submenuanim = 1.0 - std::min(1.0, std::abs(m_submenuAnim.get()-songlist_menu.getSubmenuLevel()));
	// Determine from which item to start
	int start_i = std::min(static_cast<int>(songlist_menu.curIndex() - 1), static_cast<int>(opts.size() - showopts + (songlist_menu.getSubmenuLevel() == 2 ? 1 : 0))); // Hack to counter side-effects from displaying the value inside the menu
	if (start_i < 0 || opts.size() == showopts) start_i = 0;

	// Loop the currently visible options
	for (unsigned i = static_cast<unsigned>(start_i), ii = 0; ii < showopts && i < static_cast<unsigned>(opts.size()); ++i, ++ii) {
		MenuOption const& opt = opts[i];
		ColorTrans c(window, Color::alpha(static_cast<float>(submenuanim)));

		// Selection
		if (i == songlist_menu.curIndex()) {
			// Animate selection higlight moving
			double selanim = m_selAnim.get() - start_i;
			if (selanim < 0.0) selanim = 0.0;
			// Draw the text, dim if option not available
			{
				ColorTrans c(window, Color::alpha(opt.isActive() ? 1.0f : 0.5f));
				theme->option_selected.dimensions.left(x).center(start_y + static_cast<float>(ii)*0.049f);
				theme->option_selected.draw(window, opt.getName());
			}
			wcounter = std::max(wcounter, theme->option_selected.w() + 2.0f * sel_margin); // Calculate the widest entry
			// If this is a config item, show the value below
			if (opt.type == MenuOption::Type::CHANGE_VALUE) {
				++ii; // Use a slot for the value
				theme->option_selected.dimensions.left(x + sel_margin).center(static_cast<float>(-0.1 + (selanim+1.0)*0.08));
				theme->option_selected.draw(window, "<  " + opt.value->getValue() + "  >");
			}

		// Regular option (not selected)
		} else {
			std::string title = opt.getName();
			SvgTxtTheme& txt = getTextObject(title);
			ColorTrans c(window, Color::alpha(opt.isActive() ? 1.0f : 0.5f));
			txt.dimensions.left(x).center(start_y + static_cast<float>(ii)*0.05f);
			txt.draw(window, title);
			wcounter = std::max(wcounter, txt.w() + 2.0f * sel_margin); // Calculate the widest entry
		}
	}
	songlist_menu.dimensions.stretch(wcounter, 1.0f);
}

SvgTxtTheme& ScreenPlaylist::getTextObject(std::string const& txt) {
	if (theme->options.find(txt) != theme->options.end()) return (*theme->options.at(txt).get());
	std::pair<std::string, std::unique_ptr<SvgTxtTheme>> kv = std::make_pair(txt, std::make_unique<SvgTxtTheme>(findFile("mainmenu_option.svg"), config["graphic/text_lod"].f()));
	theme->options.insert(std::move(kv));
	return (*theme->options.at(txt).get());
}

void ScreenPlaylist::createSongListMenu() {
	std::ostringstream oss_playlist;
	unsigned count = 1;
	songlist_menu.clear();
	SongList& currentList = getGame().getCurrentPlayList().getList();
	float totaldurationSeconds = 0.0;
	for (auto const& song: currentList) {
		//timestamp handles
		unsigned hours = 0;
		unsigned minutes = 0;
		unsigned seconds = static_cast<unsigned>(totaldurationSeconds);
		while(seconds >= 60) {
			minutes++;
			seconds -= 60;
		}
		while(minutes >= 60) {
			hours ++;
			minutes -=60;
		}
		oss_playlist << "#" << count << " : " << song->artist << " - " << song->title << "  +";
		if(hours > 0) {
			oss_playlist << std::setw(2) << std::setfill('0') << hours << ":";
		}
			oss_playlist << std::setw(2) << std::setfill('0') << minutes << ":" << std::setw(2) << std::setfill('0') << seconds;
		std::string songinfo = oss_playlist.str();
		if (songinfo.length() > 20) {
			songinfo = songinfo + "                           >"; //FIXME: ugly hack to make the text scale so it fits on screen!
		}
		//then add it to the menu:
		songlist_menu.add(MenuOption(_(songinfo.c_str()),_("Press enter to view song options"))).call([this, count]() {
			createSongMenu(count);
			overlay_menu.open();
		});
		oss_playlist.str("");
		count++;
		totaldurationSeconds += static_cast<float>(song->getDurationSeconds());
		totaldurationSeconds += static_cast<float>(config["game/playlist_screen_timeout"].ui());
	}
	songlist_menu.add(MenuOption(_("View more options"),_("View general playlist settings"))).call([this]() {
		createEscMenu();
		overlay_menu.open();
	});
}

void ScreenPlaylist::createSongMenu(unsigned songNumber) {
	overlay_menu.clear();
	std::string firstOption = songNumber >= 2 ? _("Play first") : _("Continue");
	std::string firstDesc = songNumber >= 2 ?
		_("Ignore the playlist's order and play this song first") :
		_("Start the song already!");

	auto& game = getGame();

	overlay_menu.add(MenuOption(firstOption, firstDesc)).call([songNumber, &game]() {
		Screen* s = game.getScreen("Sing");
		ScreenSing* ss = dynamic_cast<ScreenSing*>(s);
		assert(ss);
		ss->setSong(game.getCurrentPlayList().getSong(songNumber - 1u));
		game.activateScreen("Sing");
	});
	overlay_menu.add(MenuOption(_("Remove"), _("Remove this song from the list"))).call([this, songNumber]() {
		// Minus 1 so it doesn´t remove #2 when you´ve selected #1
		getGame().getCurrentPlayList().removeSong(songNumber - 1u);
		overlay_menu.close();
		if (getGame().getCurrentPlayList().isEmpty()) {
			getGame().activateScreen("Songs");
		} else {
			createSongListMenu();
		}
	});
	if (songNumber >= 2u) { //can't move up first song
		overlay_menu.add(MenuOption(_("Move up"), _("Move this song up the list"))).call([this, songNumber]() {
			getGame().getCurrentPlayList().swap(songNumber -1u, songNumber -2u);
			createSongListMenu();
			overlay_menu.close();
		});
	}
	unsigned size = static_cast<unsigned>(getGame().getCurrentPlayList().getList().size());
	if (songNumber < size) { //can't move down the last song
		overlay_menu.add(MenuOption(_("Move down"), _("Move this song down the list"))).call([this, songNumber]() {
			getGame().getCurrentPlayList().swap(songNumber -1, songNumber);
			createSongListMenu();
			overlay_menu.close();
		});
	}
	overlay_menu.add(MenuOption(_("Back"), _("Back to playlist viewer"))).call([this]() {
		overlay_menu.close();
	});
}

void ScreenPlaylist::triggerSongListUpdate() {
std::lock_guard<std::mutex> l (m_mutex);
needsUpdate = true;
}
