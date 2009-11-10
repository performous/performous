#pragma once

#include <string>

#include "players.hh"
#include "hiscore.hh"

#include "fs.hh"

/**Access to a database for performous which holds
  Player-, Hiscore-, Song- and Partydata.

  Will be initialized at the very beginning of
  the program.*/
class Database
{
public:
	Database (fs::path filename);
	~Database ();

	void load();
	void save();

	std::string file();

private:
	fs::path m_filename;

public: // TODO make private
	Players m_players;
	Hiscore m_hiscore;
};
