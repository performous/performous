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

	/**Operator for sorting by score.*/
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

	void addHiscore(int score, int playerid, int songid, std::string const& track);
private:
	typedef std::multiset<HiscoreItem>hiscore_t;

	hiscore_t m_hiscore;
};
