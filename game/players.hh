#pragma once
#include <list>
#include <string>


/** Static Information of a player, not
  dependent from current song.

  Used for Players Management.
  */
struct PlayerItem
{
	std::string name; /// unique name, link to highscore
/* Future ideas
	std::string displayedName; /// artist name, short name, nick (can be changed)
	std::string picture; /// a path to a picture shown
	std::map<std::string, int> scores; /// map between a Song and the highest score the Player achieved
*/
};

/**A collection of all Players.
 
 The current players plugged in a song can
 be retrieved with Engine::getPlayers().*/
class Players
{
  public:
	typedef std::list<PlayerItem> players_t;
  private:
	players_t m_players;
	std::string m_filename;
  public:
	Players(std::string filename);
	~Players();
	void load();
	void save();
	void addPlayer (std::string name);
};
