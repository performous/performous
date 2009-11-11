#pragma once

#include <string>

#include "players.hh"
#include "hiscore.hh"
#include "songitems.hh"

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

	/**Loads the whole database from xml.
	  @exception bad_cast may be thrown if xml element is not of correct type
	  @exception xmlpp exceptions may be thrown on any parse errors
	  @exception PlayersException if some conditions of players fail (e.g. no id)
	  @exception HiscoreException if some hiscore conditions fail (e.g. score too high)
	  @exception SongItemsExceptions if some songs conditions fail (e.g. no id)
	  @post filled database
	  */
	void load();
	void save();

	std::string file();

  public: // methods for player management

	void addPlayer (std::string const& name, std::string const& picture = "", int id = -1);
	void addSong (boost::shared_ptr<Song> s);
	void addHiscore (boost::shared_ptr<Song> s);

  private:
	fs::path m_filename;

	Players m_players;
	Hiscore m_hiscores;
	SongItems m_songs;
};
