#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>

#include <ostream>

/**Exception which will be thrown when loading or
  saving a SongHiscore fails.*/
struct HiscoreException: public std::runtime_error {
	HiscoreException (std::string const& msg, unsigned int linenum) :
		runtime_error(msg), m_linenum(linenum)
	{}
	/**Line information where the problem occured.*/
	unsigned int line() const {return m_linenum;}
  private:
	unsigned int m_linenum;
};

/**This struct holds together information for a
  single item of a highscore.*/
struct SongHiscoreItem {
	std::string name;
	int score;
	// std::string song;

	/**Operator for sorting by score.*/
	bool operator < (SongHiscoreItem const& other) const
	{
		return other.score < score;
	}
};

/**SongHiscore loads (and saves) a list of SongHiscoreItems.

 The class is only used for reading information in
 performous.

 Format of highscore is compatible with Ultrastar.
 
 Name of File: High.sco
 @code
#PLAYER0:name 
#SCORE0:1234 
 @endcode
 Then the same thing for PLAYER/SCORE with other numbers up to 9.
 Finally, you need to have a line that says only "E".
 */
class SongHiscore {
  public:
	/**Open the file in a read-only mode
	  and read everything in.*/
	SongHiscore (std::string const& path_, std::string const& filename_);

	/**Close the file.*/
	~SongHiscore ();

	/**Read everything in (again).
	 Will be called at the constructor.*/
	void reload();

	/**Save the players information in High.sco
	  format to the stream os.*/
	void save (std::ostream & os);

	/**Write the players information in a pretty
	  print format to the stream os.*/
	void getInfo(std::ostream & os);

	/**Check if you reached a new highscore.

	  You must be in TOP 3 to enter the highscore list.
	  This is because it will take forever to fill all 9.
	  And people refuse to enter their names if they are not close to the top.

	  @param score is a value between 0 and 10000
	    values below 500 will lead to returning false
	  @return true if the score make it into the top.
	  @return false if addNewHiscore does not make sense
	    for that score.*/
	bool reachedNewHiscore(int score)
	{
		if (score < 500) return false;
		return score > m_scores[2].score;
	}
	/**Add a new entry to the highscore.
	  It will do nothing on empty name.
	 */
	void addNewHiscore(std::string name, int score);
  private:
	static const int m_maxEntries = 10;
	std::string m_path;
	std::string m_filename;
	std::vector <SongHiscoreItem> m_scores;
	std::ifstream m_handle;
};
