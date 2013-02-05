#include "database.hh"
#include "i18n.hh"

#include <iostream>
#include <libxml++/libxml++.h>

Database::Database(fs::path const& filename): m_filename(filename) {
	load();
}

Database::~Database() {
	save();
}

void Database::load() {
	if (!exists(m_filename)) return;
	try {
		xmlpp::DomParser domParser(m_filename.string());
		xmlpp::Node* nodeRoot = domParser.get_document()->get_root_node();
		m_players.load(nodeRoot->find("/performous/players/player"));
		m_songs.load(nodeRoot->find("/performous/songs/song"));
		m_hiscores.load(nodeRoot->find("/performous/databases/database"));
	} catch (std::exception& e) {
		std::clog << "database/error: Error loading " + m_filename.string() + ": " + e.what() << std::endl;
	}
}

void Database::save() {
	fs::path tmp = m_filename.string() + ".tmp";
	try {
		xmlpp::Document doc;
		xmlpp::Node* nodeRoot = doc.create_root_node("performous");
		m_players.save(nodeRoot->add_child("players"));
		m_songs.save(nodeRoot->add_child("songs"));
		m_hiscores.save(nodeRoot->add_child("databases"));

		if (!m_filename.parent_path().empty() && !exists(m_filename.parent_path())) {
			create_directory(m_filename.parent_path());
		}
		doc.write_to_file_formatted(tmp.string(), "UTF-8");
	} catch (std::exception const& e) {
		std::clog << "database/error: Could not save " + m_filename.string() + ": " + e.what() << std::endl;
		return;
	}
	try {
		rename(m_filename.string() + ".tmp", m_filename);
	} catch (std::exception const& e) {
		std::clog << "database/error: Unable to rename temporary file to " + m_filename.string() + ": " + e.what() << std::endl;
	}
}

void Database::addPlayer(std::string const& name, std::string const& picture, int id) {
	m_players.addPlayer(name, picture, id);
}

void Database::addSong(boost::shared_ptr<Song> s) {
	m_songs.addSong(s);
}

void Database::addHiscore(boost::shared_ptr<Song> s) {
	int playerid = m_players.lookup(m_players.current().name);
	int score = scores.front().score;
	std::string track = scores.front().track;
	int songid = m_songs.lookup(s);

	m_hiscores.addHiscore(score, playerid, songid, track);
}

bool Database::reachedHiscore(boost::shared_ptr<Song> s) const {
	int score = scores.front().score;
	std::string track = scores.front().track;
	int songid = m_songs.lookup(s);

	return m_hiscores.reachedHiscore(score, songid, track);
}

void Database::queryOverallHiscore(std::ostream & os, std::string const& track) const {
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

void Database::queryPerSongHiscore(std::ostream & os, boost::shared_ptr<Song> s, std::string const& track) const {
	int songid = m_songs.lookup(s);
	std::vector<HiscoreItem> hi = m_hiscores.queryHiscore(10, -1, songid, track);

	if (songid == -1 || hi.empty()) {
		os << _("No Items up to now.") << '\n';
		os << _("Be the first to be listed here!") << '\n';
		return;
	}

	for (size_t i=0; i<hi.size(); ++i) {
		os << i+1 << ".\t"
		   << m_players.lookup(hi[i].playerid) << "\t"
		   << hi[i].score << "\t"
		   << "(" << hi[i].track << ")\n";
	}
}

void Database::queryPerSongHiscore_HiscoreDisplay(std::ostream & os, boost::shared_ptr<Song> s, int& start_pos, unsigned max_displayed, std::string const& track) const {
	int songid = m_songs.lookup(s);
	std::vector<HiscoreItem> hi = m_hiscores.queryHiscore(10, -1, songid, track);

	if (songid == -1 || hi.empty()) {
		os << _("No Items up to now.") << '\n';
		os << _("Be the first to be listed here!") << '\n';
		return;
	}

	// Limits
	if (start_pos > (int)hi.size() - (int)max_displayed) start_pos = hi.size() - max_displayed;
	if (start_pos < 0 || hi.size() <= max_displayed) start_pos = 0;

	for (size_t i = 0; i < hi.size() && i < max_displayed; ++i) {
		os << i+start_pos+1 << "\t"
		   << m_players.lookup(hi[i+start_pos].playerid) << "\t"
		   << hi[i+start_pos].score << "\t"
		   << "(" << hi[i+start_pos].track << ")\n";
	}
}

void Database::queryPerPlayerHiscore(std::ostream & os, std::string const& track) const {
	int playerid = m_players.lookup(m_players.current().name);
	std::vector<HiscoreItem> hi = m_hiscores.queryHiscore(3, playerid, -1, track);
	for (size_t i=0; i<hi.size(); ++i) {
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

