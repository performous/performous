#pragma once

#include "screen.hh"
#include "isongfilter.hh"
#include "theme.hh"
#include "ui/button.hh"
#include "ui/form.hh"
#include "ui/formscreen.hh"
#include "ui/graphiccontext.hh"
#include "ui/label.hh"
#include "ui/select.hh"
#include "ui/textbox.hh"

class Game;
class Songs;

class ScreenSongFilter : public FormScreen {
  public:
	ScreenSongFilter(Game&, Songs&);
	~ScreenSongFilter() override = default;

	void onExitSwitchTo(std::string const&);

	void prepare() override {}
	void draw() override;
	void enter() override;
	void exit() override;
	void reloadGL() override {}

  protected:
	void onCancel() override;
	void onAfterEventProcessing() override;

  private:
	void initializeControls();
	void updateResult();
	void resetFilter();
	SongFilterPtr makeFilter() const;

  private:
	Game& m_game;
	Songs& m_songs;
	std::unique_ptr<ThemeSongFilterScreen> m_theme;
	std::string m_nextScreen = "Intro";
	Label m_labelLanguage;
	Select m_selectLanguage0;
	Select m_selectLanguage1;
	Select m_selectLanguage2;
	Label m_labelGenre;
	Select m_selectGenre0;
	Select m_selectGenre1;
	Select m_selectGenre2;
	Label m_labelYear;
	Select m_selectYear0;
	Select m_selectYear1;
	Select m_selectYear2;
	Label m_labelMode;
	Select m_selectMode;
	Label m_labelTitle;
	TextBox m_textBoxTitle;
	Label m_labelArtist;
	TextBox m_textBoxArtist;
	Label m_labelBroken;
	Select m_selectBroken;
	Label m_labelResults;
	Button m_buttonReset;
};


