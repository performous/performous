#pragma once

#include <vector>
#include <string>

#include <boost/noncopyable.hpp>

#include "player.hh"
#include "animvalue.hh"
#include "fs.hh"

namespace xmlpp { class Node; class Element; typedef std::vector<Node*>NodeSet; }


/**A collection of all Players.
 
 The current players plugged in a song can
 be retrieved with Engine::getPlayers().*/
class Players: boost::noncopyable {
  private:
	typedef std::vector<PlayerItem> players_t;
	typedef std::list<Player> cur_players_t;
	typedef std::list<int> cur_scores_t;

  private:
	players_t m_players;
	players_t m_filtered;

	std::string m_filter;
	AnimAcceleration math_cover;

	bool m_dirty;

  public:
	cur_players_t cur;
	cur_scores_t scores;

  public:
	Players();
	~Players();

	void load(xmlpp::NodeSet n);
	void save(xmlpp::Element *players);

	void update();

	/// add a player with a displayed name and an optional picture; if no id is given one will be assigned
	void addPlayer (std::string const& name, std::string const& picture = "", int id = -1);

	/// const array access
	PlayerItem operator[](std::size_t pos) const {
		if (pos < m_filtered.size()) return m_filtered[pos];
		else return PlayerItem();
	}
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
	PlayerItem current() const {
		if (math_cover.getTarget() < m_filtered.size()) return m_filtered[math_cover.getTarget()];
		else return PlayerItem();
	}
	/// filters playerlist by regular expression
	void setFilter(std::string const& regex);
  private:
	void filter_internal();
};
