#pragma once

class Database;

#include "animvalue.hh"
#include "instruments.hh"
#include "progressbar.hh"
#include "texture.hh"
#include "theme.hh"

/// shows score at end of song
class ScoreWindow {
  public:
	ScoreWindow(Instruments& instruments, Database& database);
    
	/// draws ScoreWindow
	void draw();
	bool empty();
    
  private:
	Database& m_database;
	AnimValue m_pos;
	Texture m_bg;
	ProgressBar m_scoreBar;
	SvgTxtThemeSimple m_score_text;
	SvgTxtTheme m_score_rank;
	std::string m_rank;
};

