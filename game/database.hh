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

public: // methods for player management

	void addPlayer (std::string const& name, std::string const& picture = "", int id = -1)
	{ m_players.addPlayer(name, picture, id); }

private:
	fs::path m_filename;

	Players m_players;
	Hiscore m_hiscores;
};
