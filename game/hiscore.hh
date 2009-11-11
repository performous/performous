#pragma once

#include <set>
#include <vector>
#include <stdexcept>

namespace xmlpp { class Node; class Element; typedef std::vector<Node*>NodeSet; }

/**Exception which will be thrown when loading or
  saving a SongHiscore fails.*/
struct HiscoreException: public std::runtime_error {
	HiscoreException (std::string const& msg) :
		runtime_error(msg)
	{}
};

/**This struct holds together information for a
  single item of a highscore.*/
struct HiscoreItem {
	int score;

	int playerid;
	int songid;

	std::string track;

	/**Operator for sorting by score.
	 Reverse order, so that highest is first!*/
	bool operator < (HiscoreItem const& other) const
	{
		return other.score < score;
	}
};

class Hiscore
{
public:
	Hiscore ();

	void load(xmlpp::NodeSet const& n);
	void save(xmlpp::Element *players);

	/**Check if you reached a new highscore.

	  You must be in TOP 3 of a specific song to enter the highscore list.
	  This is because it will take forever to fill more.
	  And people refuse to enter their names if they are not close to the top.

	  @param score is a value between 0 and 10000
	    values below 500 will lead to returning false
	  @return true if the score make it into the top.
	  @return false if addNewHiscore does not make sense
	    for that score.*/
	bool reachedHiscore(int score, int songid, std::string const& track = "VOCALS");
	void addHiscore(int score, int playerid, int songid, std::string const& track = "VOCALS");
private:
	typedef std::multiset<HiscoreItem>hiscore_t;

	hiscore_t m_hiscore;
};
