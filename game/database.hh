#pragma once

#include "color.hh"
#include "controllers.hh"
#include "fs.hh"
#include "hiscore.hh"
#include "players.hh"
#include "scoreitem.hh"
#include "songitems.hh"
#include <optional>
#include <string>
#include <ostream>

/**Access to a database for performous which holds
  Player-, Hiscore-, Song-, Track- and (in future)
  Partydata.

  This is a facade for Players, Hiscore and SongItems.

  Will be initialized at the very beginning of
  the program.

  The current lists (Players and scores) are used
  to pass the information which players have won
  to the ScoreScreen and then to the players window.
 */
class Database {
public:
	/**Will try to load the database.
	  If it does not succeed the error will be ignored.
	  Only some information will be printed on stderr.
	  */
	Database(fs::path const& filename);
	/**Will try to save the database.
	  This will even be done if the loading failed.
	  It tries to create the directory above the file.
	  */
	~Database();

	/**Loads the whole database from xml.
	  @exception bad_cast may be thrown if xml element is not of correct type
	  @exception xmlpp exceptions may be thrown on any parse errors
	  @exception PlayersException if some conditions of players fail (e.g. no id)
	  @exception HiscoreException if some hiscore conditions fail (e.g. score too high)
	  @exception SongItemsExceptions if some songs conditions fail (e.g. no id)
	  @post filled database
	  */
	void load();
	/**Saves the whole database to xml.
	  Will write out everything to the file given in the constructor, @see file()
	*/
	void save();

	friend class ScreenHiscore;
	friend class ScreenPlayers;
	friend class ScreenSing;
	friend class ScoreWindow;
	friend class LayoutSinger;
	friend class NoteGraph;
	friend class Engine;
private: // will be bypassed by above friend declaration
	typedef std::list<Player> cur_players_t;
	typedef std::list<ScoreItem> cur_scores_t;

	//This fields are misused as additional parameters
	cur_players_t cur;
	cur_scores_t scores;

public: // methods for database management

	/**A facade for Players::addPlayer.*/
	void addPlayer(std::string const& name, std::string const& picture = "", std::optional<PlayerId> id = std::nullopt);
	/**A facade for SongItems::addSong.*/
	void addSong(std::shared_ptr<Song> s);
	/**A facade for Hiscore::addHiscore.
	 The ids will be looked up first by using the songs and current players data.
	 */
	void addHiscore(std::shared_ptr<Song> s);

public: // methods for database queries
	/**A facade for Hiscore::reachedHiscore.
	  Queries if the current player with current score has reached a new hiscore
	  for the song s.
	 */
	bool reachedHiscore(std::shared_ptr<Song> s) const;

	void queryOverallHiscore(std::ostream & os, std::string const& track = std::string()) const;
	void queryPerSongHiscore(std::ostream & os, std::shared_ptr<Song> s, std::string const& track = std::string()) const;
	void queryPerPlayerHiscore(std::ostream & os, std::string const& track = std::string()) const;

	bool hasHiscore(Song const& s) const;
	unsigned getHiscore(const Song& s) const;
	bool noPlayers() const;

private:
	fs::path m_filename;

	Players m_players;
	Hiscore m_hiscores;
	SongItems m_songs;
};
