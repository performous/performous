#include "database.hh"

#include <iostream>

#include <libxml++/libxml++.h>

Database::Database(fs::path filename) :
	m_filename(filename)
{
	try { load(); }
	catch(std::exception const& e) {
		std::cerr << "Could not load " << file() << ": " << e.what() << std::endl;
		std::cerr << "will try to create" << std::endl;
	}
}

Database::~Database() {
	try { save(); }
	catch (std::exception const& e) {
		std::cerr << "Could not save " << file() << ": " << e.what() << std::endl;
	}
}

void Database::load() {
	if (!exists(m_filename)) return;
	xmlpp::DomParser domParser(m_filename.string());
	xmlpp::Node* nodeRoot = domParser.get_document()->get_root_node();

	xmlpp::NodeSet players = nodeRoot->find("/performous/players/player");
	m_players.load(players);

	xmlpp::NodeSet hiscores = nodeRoot->find("/performous/hiscores/hiscore");
	m_hiscores.load(hiscores);

	xmlpp::NodeSet songs = nodeRoot->find("/performous/songs/song");
	m_songs.load(songs);
}

void Database::save() {
	xmlpp::Document doc;
	xmlpp::Node* nodeRoot = doc.create_root_node("performous");

	xmlpp::Element *players = nodeRoot->add_child("players");
	m_players.save(players);

	xmlpp::Element *hiscores = nodeRoot->add_child("hiscores");
	m_hiscores.save(hiscores);

	xmlpp::Element *songs = nodeRoot->add_child("songs");
	m_songs.save(songs);

	if (!exists(m_filename.parent_path()) && !m_filename.parent_path().empty())
	{
		std::cout << "Will create directory: " << m_filename.parent_path() << std::endl;
		create_directory(m_filename.parent_path());
	}
	doc.write_to_file_formatted(m_filename.string(), "UTF-8");
}

std::string Database::file() {
	return m_filename.string();
}

void Database::addPlayer (std::string const& name, std::string const& picture, int id) {
	m_players.addPlayer(name, picture, id);
}

void Database::addSong (boost::shared_ptr<Song> s) {
	m_songs.addSong(s);
}

void Database::addHiscore (boost::shared_ptr<Song> s) {
	int playerid = m_players.lookup(m_players.current().name);
	int score = scores.front().score;
	std::string track = scores.front().track;
	int songid = m_songs.lookup(s);

	m_hiscores.addHiscore(score, playerid, songid, track);
}

bool Database::reachedHiscore (boost::shared_ptr<Song> s) const {
	int score = scores.front().score;
	std::string track = scores.front().track;
	int songid = m_songs.lookup(s);

	return m_hiscores.reachedHiscore(score, songid, track);
}

void Database::queryOverallHiscore (std::ostream & os, std::string const& track) const {
	std::vector<HiscoreItem> hi = m_hiscores.queryHiscore (10, -1, -1, track);
	for (size_t i=0; i<hi.size(); ++i)
	{
		os << i+1 << ".\t"
		   << m_players.lookup(hi[i].playerid) << "\t"
		   << m_songs.lookup(hi[i].songid) << "\t"
		// << hi[i].track << "\t"
		   << hi[i].score << "\n";
	}
}

void Database::queryPerSongHiscore (std::ostream & os, boost::shared_ptr<Song> s, std::string const& track) const {
	int songid = m_songs.lookup(s);
	std::vector<HiscoreItem> hi = m_hiscores.queryHiscore(10, -1, songid, track);

	if (songid == -1 || hi.size() == 0)
	{
		os << "No Items up to now.\n";
		os << "Be the first to be listed here!\n";
		return;
	}

	for (size_t i=0; i<hi.size(); ++i)
	{
		os << i+1 << ".\t"
		   << m_players.lookup(hi[i].playerid) << "\t"
		   << hi[i].score << "\t"
		   << "(" << hi[i].track << ")\n";
	}
}

void Database::queryPerSongHiscore_HiscoreDisplay (std::ostream & os, boost::shared_ptr<Song> s, int& start_pos, unsigned max_displayed, std::string const& track) const {
	int songid = m_songs.lookup(s);
	std::vector<HiscoreItem> hi = m_hiscores.queryHiscore(10, -1, songid, track);

	if (songid == -1 || hi.size() == 0)
	{
		os << "No Items up to now.\n";
		os << "Be the first to be listed here!\n";
		return;
	}

	// Limits
	if (start_pos > (int)hi.size() - (int)max_displayed) start_pos = hi.size() - max_displayed;
	if (start_pos < 0 || hi.size() <= max_displayed) start_pos = 0;

	for (size_t i = 0; i < hi.size() && i < max_displayed; ++i)
	{
		os << i+start_pos+1 << "\t"
		   << m_players.lookup(hi[i+start_pos].playerid) << "\t"
		   << hi[i+start_pos].score << "\t"
		   << "(" << hi[i+start_pos].track << ")\n";
	}
}

void Database::queryPerPlayerHiscore (std::ostream & os, std::string const& track) const {
	int playerid = m_players.lookup(m_players.current().name);
	std::vector<HiscoreItem> hi = m_hiscores.queryHiscore(3, playerid, -1, track);
	for (size_t i=0; i<hi.size(); ++i)
	{
		os << i+1 << ".\t"
		   << m_songs.lookup(hi[i].songid) << "\t"
		   << hi[i].score << "\t"
		   << "(" << hi[i].track << ")\n";
	}
}

bool Database::hasHiscore(Song& s) const {
	int songid = m_songs.lookup(s);
	return m_hiscores.hasHiscore(songid);
}

/*
int test(std::string const& name, std::string const& song, int score) {
	Database d("database.xml");
	// d.addPlayer("Markus", "m.jpg");

	boost::shared_ptr<Song> s(new Song("/usr/share/songs/ABBA/ABBA - " + song + "/", "ABBA - " + song + ".txt"));
	d.addSong(s);

	PlayerItem pi;
	pi.name = name;
	d.m_players.m_filtered.push_back(pi);
	d.scores.push_back(score);

	if (d.reachedHiscore(s))
	{
		std::cout << "Reached a new Hiscore" << std::endl;
		d.addHiscore(s);
	}

	std::cout << " --- Overall Hiscore ---" << std::endl;
	d.queryOverallHiscore(std::cout);

	std::cout << " --- Player Hiscore ---" << std::endl;
	d.queryPerPlayerHiscore(std::cout);

	std::cout << " --- Song Hiscore ---" << std::endl;
	d.queryPerSongHiscore(std::cout, s);

	return 0;
}

#include "boost/lexical_cast.hpp"

int main(int argc, char**argv) {

	if (argc < 4) return 3;

	std::string name = argv[1];
	std::string song = argv[2];
	int score = boost::lexical_cast<int>(argv[3]);

	return test(name, song,  score);
}
*/
