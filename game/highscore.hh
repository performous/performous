#pragma once

#include <string>
#include <vector>
#include <stdexcept>

struct HighScoreException: public std::runtime_error {
	HighScoreException (std::string const& msg, unsigned int linenum) :
		runtime_error(msg), m_linenum(linenum)
	{}
	unsigned int line() const {return m_linenum;}
  private:
	unsigned int m_linenum;
};

class HighScore {
  public:
	HighScore (std::string const& path_, std::string const& filename_);
	~HighScore ();

	void save();
  private:
	std::string m_path;
	std::string m_filename;
	std::vector <std::string> m_names;
	std::vector <double> m_scores;
};
