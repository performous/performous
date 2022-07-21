#pragma once

#include <set>
#include <list>
#include <optional>
#include <vector>
#include <string>
#include <stdexcept>


#include "player.hh"
#include "unicode.hh"
#include "animvalue.hh"
#include "libxml++.hh"

/**Exception which will be thrown when loading or
  saving Players fails.*/
struct PlayersException: public std::runtime_error {
	PlayersException (std::string const& msg) :
		runtime_error(msg)
	{}
};

/**A collection of all Players.

 The current players plugged in a song can
 be retrieved with Engine::getPlayers().

 There are 3 different views united in that collection.
 There is a full players list which are used by the
 database, but also for the filtering.

 The filtered list is used to show players in
 the screen_players.
 */
class Players {
  private:
	typedef std::set<PlayerItem> players_t;
	typedef std::vector<PlayerItem> fplayers_t;

  public:
	Players(const Players&) = delete;
  	const Players& operator=(const Players&) = delete;
	Players() = default;

	void load(xmlpp::NodeSet const& n);
	void save(xmlpp::Element *players);

	void update();

	/// lookup a playerid using the players name
	std::optional<PlayerId> lookup(std::string const& name) const;

	/** lookup a players name using the playerid.
	  @return an optional with the players
	  */
	std::optional<std::string> lookup(const PlayerId &id) const;

	/// add a player with a displayed name and an optional picture; if no id is given one will be assigned
	void addPlayer (std::string const& name, std::string const& picture = "", std::optional<PlayerId> id = std::nullopt);

	/// const array access
	PlayerItem operator[](unsigned pos) const;
	unsigned count() const { return static_cast<unsigned>(m_filtered.size()); }
	bool isEmpty() const { return m_filtered.empty(); }
	/// advances to next player
	void advance(std::ptrdiff_t diff);
	/// get current id
	std::optional<PlayerId> currentId() const { return static_cast<unsigned>(math_cover.getTarget()); }
	/// gets current position
	double currentPosition() { return math_cover.getValue(); }
	/// gets current velocity
	double currentVelocity() const { return math_cover.getVelocity(); }
	/// sets margins for animation
	void setAnimMargins(double left, double right) { math_cover.setMargins(left, right); }
	/// @return current PlayerItem (the copy is very cheap at the moment)
	PlayerItem current() const;
	/// filters playerlist by regular expression
	void setFilter(std::string const& regex);
  
private:
	PlayerId assign_id_internal(); /// returns the next available id
	void filter_internal();
	static icu::ErrorCode m_icuError;
	static icu::RuleBasedCollator icuCollator;

private:
	players_t m_players;
	fplayers_t m_filtered;

	std::string m_filter;
	AnimAcceleration math_cover;

	bool m_dirty = false;
};
