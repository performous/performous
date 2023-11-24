#pragma once

#include "animvalue.hh"
#include "menu.hh"
#include "screen.hh"
#include "theme/theme.hh"
#include "event_manager.hh"

class Audio;
class ThemeIntro;
class SvgTxtTheme;
class MenuOption;

/// intro screen
class ScreenIntro : public Screen {
  public:
	/// constructor
	ScreenIntro(Game &game, std::string const& name, Audio& audio);
	void enter();
	void exit();
	void reloadGL();
	void manageEvent(SDL_Event event);
	void manageEvent(input::NavEvent const& event);
	void draw();

  private:
	void draw_menu_options();
	void draw_webserverNotice();
	void populateMenu();
	SvgTxtTheme& getTextObject(std::string const& txt);
	void onLoad(EventParameter const&);
	void onEnter(EventParameter const&);

	Audio& m_audio;
	std::shared_ptr<ThemeIntro> m_theme;
	Menu m_menu;
	bool m_first;
	AnimValue m_selAnim;
	AnimValue m_submenuAnim;
	AnimValue m_webserverNoticeTimeout;
	int webserversetting = 0;
	#ifdef USE_WEBSERVER
	bool m_drawNotice = false;
	#endif
};
