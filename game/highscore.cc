#include "highscore.hh"
#include <boost/lexical_cast.hpp>

#include "unicode.hh"

#include <fstream>
#include <sstream>
#include <algorithm>

HighScore::HighScore (std::string const& path_, std::string const& filename_) :
	m_path(path_),
	m_filename(filename_),
	m_scores(m_maxEntries)
{}

HighScore::~HighScore()
{}

void HighScore::load()
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
		m_scores[playernum].name = name;

		std::getline(ss, str);
		linenum++;
		if (str.empty()) throw HighScoreException("Expected Score, but found empty line", linenum);
		if (str.length() <= 8) throw HighScoreException("Line not long enough for score information", linenum);

		if (str[0] != '#') throw HighScoreException("No # found at begin of line", linenum);
		if (str.substr(1,5) != "SCORE") throw HighScoreException("Expected SCORE not found", linenum);

		nr = str[6];
		if (nr != playernum + '0') throw HighScoreException("Did not find correct playernum", linenum);
		if (str[7] != ':') throw HighScoreException("Did not find :", linenum);

		std::string sscore = str.substr(8, str.length()-8);
		if (sscore.empty()) throw HighScoreException("Did not find score", linenum);

		int score = 0;
		try {
			score = boost::lexical_cast<int>(sscore);
		} catch (boost::bad_lexical_cast const& blc)
		{
			throw HighScoreException("Did not find valid int number", linenum);
		}
		if (score < 0 || score > 10000) throw HighScoreException("Number not between 0 and 10000", linenum);
		if (playernum>0 && m_scores[playernum-1].score < score) throw HighScoreException("Lower ranked highscore is higher", linenum);
		m_scores[playernum].score = score;


		playernum++;
	}
}

void HighScore::save()
{
	std::ofstream out ((m_path + m_filename).c_str());

	if (!out.is_open()) throw HighScoreException("Could not open file for writing", 0);

	out.exceptions ( std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit );
	for (size_t i=0; i<m_scores.size();i++)
	{
		if (m_scores[i].score <= 0) break; // maybe change to 500?

		try {
			out << "#PLAYER" << i << ":"
			  << m_scores[i].name << std::endl;
			out << "#SCORE" << i << ":"
			  << m_scores[i].score << std::endl;
		} catch (std::ofstream::failure const&) {
			throw HighScoreException("Unexpected I/O error", i);
		}
	}
	out << "E" << std::endl;
}

void HighScore::addNewHighscore(std::string name, int score)
{
	// do not allow invalid name
	if (name == "") return;

	HighScoreItem hsi;
	hsi.name = name;
	hsi.score = score;

	m_scores.push_back(hsi);
	std::sort(m_scores.begin(), m_scores.end());

	m_scores.resize(m_maxEntries); // throw away worst score
}

/*
#include <iostream>

int main()
{
	try {
		HighScore hi ("", "highscore.txt");
		hi.load();
		int new_score = 9000;
		if (hi.reachedNewHighscore(new_score))
		{
			std::cout << "Reached new highscore" << std::endl;
			hi.addNewHighscore("new player", new_score);
		}
		hi.save();
	} catch (HighScoreException const& hie)
	{
		std::cerr << "Exception: " << hie.what()
			<< "  at line: " << hie.line()
			<< std::endl;
	}
}
*/
