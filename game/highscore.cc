#include "highscore.hh"
#include <boost/lexical_cast.hpp>

#include "unicode.hh"

#include <fstream>
#include <sstream>

HighScore::HighScore (std::string const& path_, std::string const& filename_) :
	m_path(path_),
	m_filename(filename_)
{
	std::ifstream in((m_path + m_filename).c_str());

	if (!in.is_open()) throw HighScoreException("Could not open highscore file", 0);


	in.seekg(0, std::ios::end);
	size_t size = in.tellg();
	if (size < 10 || size > 100000) throw HighScoreException("Does not look like a highscore file (wrong size)", 1);
	in.seekg(0);

	std::vector<char> data(size);
	if (!in.read(&data[0], size)) throw HighScoreException("Unexpected I/O error", 0);

	std::stringstream ss;
	ss.write(&data[0], size);

	convertToUTF8(ss, m_path + m_filename);

	// now parse line by line and build up highscore
	std::string str;
	unsigned int linenum = 0;
	unsigned int playernum = 0;
	while (std::getline(ss, str))
	{
		if (str == "E") break;
		linenum++;
		if (playernum > 9) throw HighScoreException("Not more than 10 highscores expected in file", linenum);
		if (str.empty()) continue; // ignore empty player lines
		if (str.length() <= 9) throw HighScoreException("Line not long enough for player information", linenum);

		if (str[0] != '#') throw HighScoreException("No # found at begin of line", linenum);
		if (str.substr(1,6) != "PLAYER") throw HighScoreException("Expected PLAYER not found", linenum);

		unsigned int nr = str[7];
		if (nr != playernum + '0') throw HighScoreException("Did not find correct playernum", linenum);
		if (str[8] != ':') throw HighScoreException("Did not find :", linenum);

		std::string name = str.substr(9, str.length()-9);
		if (name.empty()) throw HighScoreException("Did not find name", linenum);
		std::cout << "name: " << name << std::endl;
		m_names.push_back(name);

		std::getline(ss, str);
		linenum++;
		if (str.empty()) throw HighScoreException("Expected Score, but found empty line", linenum);
		if (str.length() <= 8) throw HighScoreException("Line not long enough for score information", linenum);

		if (str[0] != '#') throw HighScoreException("No # found at begin of line", linenum);
		if (str.substr(1,5) != "SCORE") throw HighScoreException("Expected SCORE not found", linenum);

		nr = str[6];
		if (nr != playernum + '0') throw HighScoreException("Did not find correct playernum", linenum);
		if (str[7] != ':') throw HighScoreException("Did not find :", linenum);

		std::string score = str.substr(8, str.length()-8);
		if (score.empty()) throw HighScoreException("Did not find score", linenum);
		std::cout << "score: " << score << std::endl;

		try {
			double dscore = boost::lexical_cast<int>(score) / 10000.0;
			if (dscore < 0.0 || dscore > 1.0) throw HighScoreException("Number not between 0 and 10000", linenum);
			if (playernum>0 && m_scores.back() < dscore) throw HighScoreException("Lower ranked highscore is higher", linenum);
			m_scores.push_back(dscore);
		} catch (boost::bad_lexical_cast const& blc)
		{
			throw HighScoreException("Did not find valid double number", linenum);
		}

		playernum++;
	}
}

HighScore::~HighScore()
{}

/*
#include <iostream> // TODO debug

int main()
{
	try {
		HighScore hi ("", "highscore.txt");
	} catch (HighScoreException const& hie)
	{
		std::cerr << "Exception: " << hie.what()
			<< "  at line: " << hie.line()
			<< std::endl;
	}
}
*/
