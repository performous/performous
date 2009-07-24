#pragma once
#include <vector>
#include <string>
#include <boost/utility.hpp>

#include "animvalue.hh"


/** Static Information of a player, not
  dependent from current song.

  Used for Players Management.
  */
struct PlayerItem {
	std::string name; /// unique name, link to highscore
/* Future ideas
	std::string displayedName; /// artist name, short name, nick (can be changed)
	std::string picture; /// a path to a picture shown
	std::map<std::string, int> scores; /// map between a Song and the highest score the Player achieved
*/

	bool operator== (PlayerItem const& pi) const
	{
		return name == pi.name;
	}
};

/**A collection of all Players.
 
 The current players plugged in a song can
 be retrieved with Engine::getPlayers().*/
class Players: boost::noncopyable {
  public:
	typedef std::vector<PlayerItem> players_t;
  private:
	players_t m_players;
	players_t m_filtered;

	std::string m_filename;
	std::string m_filter;
	AnimAcceleration math_cover;

	bool m_dirty;

  public:
	Players(std::string filename);
	~Players();

	void load();
	void reload()
	{
		m_players.clear();
		load();
	}
	void save();
	void update();
	void addPlayer (std::string const& name);

	/// const array access
	PlayerItem operator[](std::size_t pos) const { return m_filtered[pos]; }
	/// number of songs
	size_t size() const { return m_filtered.size(); };
	/// true if empty
	int empty() const { return m_filtered.empty(); };
	/// advances to next player
	void advance(int diff) {
		int size = m_filtered.size();
		if (size == 0) return;  // Do nothing if no songs are available
		int _current = size ? (int(math_cover.getTarget()) + diff) % size : 0;
		if (_current < 0) _current += m_filtered.size();
		math_cover.setTarget(_current,this->size());
	}
	/// get current id
	int currentId() const { return math_cover.getTarget(); }
	/// gets current position
	double currentPosition() { return math_cover.getValue(); }
	/// gets current velocity
	double currentVelocity() const { return math_cover.getVelocity(); }
	/// sets margins for animation
	void setAnimMargins(double left, double right) { math_cover.setMargins(left, right); }
	/// @return current PlayerItem (the copy is very cheap at the moment)
	PlayerItem current() const { return m_filtered[math_cover.getTarget()]; }
	/// filters playerlist by regular expression
	void setFilter(std::string const& regex);
  private:
	void filter_internal();
};
