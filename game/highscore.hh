#pragma once

#include <string>
#include <vector>
#include <stdexcept>

/**Exception which will be thrown when loading or
  saving a HighScore fails.*/
struct HighScoreException: public std::runtime_error {
	HighScoreException (std::string const& msg, unsigned int linenum) :
		runtime_error(msg), m_linenum(linenum)
	{}
	/**Line information where the problem occured.*/
	unsigned int line() const {return m_linenum;}
  private:
	unsigned int m_linenum;
};

/**This struct holds together information for a
  single item of a highscore.*/
struct HighScoreItem {
	std::string name;
	double score;
	// std::string song;

	/**Operator for sorting by score.*/
	bool operator < (HighScoreItem const& other) const
	{
		return other.score < score;
	}
};

/**HighScore loads and saves a list of HighScoreItems.*/
class HighScore {
  public:
	HighScore (std::string const& path_, std::string const& filename_);
	~HighScore ();

	void load();
	void save();

	/**Check if you reached a new highscore.
	  @return true if the score make it into the top.
	  @return false if addNewHighscore does not make sense
	    for that score.*/
	bool reachedNewHighscore(double score)
	{
		return score > m_scores.back().score;
	}
	/**Add a new entry to the highscore.*/
	void addNewHighscore(std::string name, double score);
  private:
	static const int m_maxEntries = 10;
	std::string m_path;
	std::string m_filename;
	std::vector <HighScoreItem> m_scores;
};
