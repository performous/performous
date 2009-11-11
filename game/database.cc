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

// Test program for Database
int main()
{
	Database d("database.xml");
	d.addPlayer("Markus", "m.jpg");

	boost::shared_ptr<Song> s(new Song("/usr/share/songs/ABBA/ABBA - Dancing Queen/", "ABBA - Dancing Queen.txt"));
	d.addSong(s);
}
