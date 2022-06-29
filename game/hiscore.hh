#pragma once

#include "hiscoreitem.hh"
#include "libxml++.hh"
#include "player.hh"
#include "songitems.hh"

#include <set>
#include <string>
#include <vector>

class Hiscore {
public:
	void load(xmlpp::NodeSet const& n);
	void save(xmlpp::Element *players);

	/**Check if you reached a new highscore.

	  You must be in TOP 3 of a specific song to enter the highscore list.
	  This is because it will take forever to fill more.
	  And people refuse to enter their names if they are not close to the top.

	  @param score is a value between 0 and 10000. values below 2000 will lead to instant disqualification.
	  @return true if the score make it into the list
	  @return false if addNewHiscore does not make sense for that score.
	  */
	bool reachedHiscore(unsigned score, SongId songid, unsigned short level, std::string const& track) const;

	/**Add a specific highscore into the list.

	  @pre Hiscore is added.

	  There is no check regarding if it is useful to add this hiscore.
	  To check this, use reachedHiscore() first.

	  The method will check if all ids are non-negative and the score
	  in its valid interval. If one of this conditions is not net a
	  HiscoreException will be raised.
	  */
	void addHiscore(unsigned score, const PlayerId& playerid, SongId songid, unsigned short level, std::string const& track);

	using HiscoreVector = std::vector<HiscoreItem>;

	/// This queries the database for a sorted vector of highscores. The defaults mean to query everything.
	/// @param max limits the number of elements returned.
	unsigned getHiscore(unsigned songid) const;
	HiscoreVector queryHiscore(std::optional<PlayerId> playerid, std::optional<SongId> songid, std::string const& track, std::optional<unsigned> max = std::nullopt) const;
	bool hasHiscore(const SongId &songid) const;
	std::size_t size() const { return m_hiscore.size(); }
private:
	using hiscore_t = std::multiset<HiscoreItem>;
	hiscore_t m_hiscore;
	unsigned short currentLevel() const;
};
