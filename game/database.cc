#include "database.hh"

#include "configuration.hh"
#include "libxml++-impl.hh"
#include "fs.hh"
#include "i18n.hh"

#include <iostream>

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
		m_hiscores.load(nodeRoot->find("/performous/hiscores/hiscore"));
		std::clog << "database/info: Loaded " << m_players.count() << " players, " << m_songs.size() << " songs and " << m_hiscores.size() << " hiscores from " << m_filename.string() << std::endl;
	} catch (std::exception& e) {
		std::clog << "database/error: Error loading " + m_filename.string() + ": " + e.what() << std::endl;
	}
}

void Database::save() {
	try {
		create_directories(m_filename.parent_path());
		fs::path tmp = m_filename.string() + ".tmp";
		{
			xmlpp::Document doc;
			auto nodeRoot = doc.create_root_node("performous");
			m_players.save(xmlpp::add_child_element(nodeRoot, "players"));
			m_songs.save(xmlpp::add_child_element(nodeRoot, "songs"));
			m_hiscores.save(xmlpp::add_child_element(nodeRoot, "hiscores"));
			doc.write_to_file_formatted(tmp.string(), "UTF-8");
		}
		rename(tmp, m_filename);
		std::clog << "database/info: Saved " << m_players.count() << " players, " << m_songs.size() << " songs and " << m_hiscores.size() << " hiscores to " << m_filename.string() << std::endl;
	} catch (std::exception const& e) {
		std::clog << "database/error: Could not save " + m_filename.string() + ": " + e.what() << std::endl;
		return;
	}
}

void Database::addPlayer(std::string const& name, std::string const& picture, std::optional<PlayerId> id) {
	m_players.addPlayer(name, picture, id);
}

void Database::addSong(std::shared_ptr<Song> s) {
	m_songs.addSong(s);
}

void Database::addHiscore(std::shared_ptr<Song> s) {
	auto maybe_playerid = m_players.lookup(m_players.current().name);
	if (!maybe_playerid.has_value()) {
		std::cerr << "database/error: cannot find player ID for player '" + m_players.current().name << "'\n";
		return;
	}
	auto playerid = maybe_playerid.value();
	unsigned score = scores.front().score;
	std::string track = scores.front().track;
	const SongId songid = m_songs.lookup(s);
	
	if(!songid.has_value()) {
		std::clog << "database/error: Invalid song ID for song: " + s->artist + " - " + s->title << std::endl;
		return;
	}
	unsigned short level = config["game/difficulty"].ui();
	m_hiscores.addHiscore(score, playerid, songid.value(), level, track);
	std::clog << "database/info: Added new hiscore " << score << " points on track " << track << " of songid " << std::to_string(songid.value()) << " level "<< level<< std::endl;
}

bool Database::reachedHiscore(std::shared_ptr<Song> s) const {
	const SongId songid = m_songs.lookup(s);

	if(!songid.has_value())
		return false;

	unsigned score = scores.front().score;
	const auto track = scores.front().track;
	SongId songid = m_songs.lookup(s);
	if (!songid) { 
		std::clog << "database/error: Invalid song ID for song: " + s->artist + " - " + s->title << std::endl;
		return false;
	}
	unsigned short level = config["game/difficulty"].ui();
	return m_hiscores.reachedHiscore(score, songid.value(), level, track);
}

void Database::queryOverallHiscore(std::ostream & os, std::string const& track) const {
	std::vector<HiscoreItem> hi = m_hiscores.queryHiscore (std::nullopt, std::nullopt, track, 10);
	for (size_t i=0; i < hi.size(); ++i) {
		os << i+1 << ".\t"
		   << m_players.lookup(hi[i].playerid).value_or("Unknown player Id " + std::to_string(hi[i].playerid)) << "\t"
		   << m_songs.lookup(hi[i].songid) << "\t"
		// << hi[i].track << "\t"
		   << hi[i].score << "\n";
	}
}

void Database::queryPerSongHiscore(std::ostream & os, std::shared_ptr<Song> s, std::string const& track) const {
	SongId songid = m_songs.lookup(s);
	if (!songid) return; // song not yet in database.
	// Reorder hiscores by track / score
	std::map<std::string, std::multiset<HiscoreItem>> scoresByTrack;
	for (HiscoreItem const& hi: m_hiscores.queryHiscore(std::nullopt, songid.value(), track, std::nullopt)) scoresByTrack[hi.track].insert(hi);
		scoresByTrack[hi.track].insert(hi);
	for (auto const& hiv: scoresByTrack) {
		os << hiv.first << ":\n";
		for (auto const& hi: hiv.second) {
			os << "\t" << hi.score << "\t" << m_players.lookup(hi.playerid).value_or("Unknown player Id " + std::to_string(hi.playerid)) << "\n";
		}
		os << "\n";
	}
}

void Database::queryPerPlayerHiscore(std::ostream & os, std::string const& track) const {
	std::optional<PlayerId> playerid = m_players.lookup(m_players.current().name);
	std::vector<HiscoreItem> hi = m_hiscores.queryHiscore(playerid, std::nullopt, track, 3);

	for (size_t i=0; i < hi.size(); ++i) {
		os << i+1 << ".\t"
		   << m_songs.lookup(hi[i].songid) << "\t"
		   << hi[i].score << "\t"
		   << "(" << hi[i].track << ")\n";
	}
}

bool Database::hasHiscore(const Song& s) const {
	const SongId songid = m_songs.lookup(s);

	if (!songid) { 
		std::clog << "database/error: Invalid song ID for song: " + s.artist + " - " + s.title << std::endl;
		return false;
	}
	return m_hiscores.hasHiscore(songid.value());
}

unsigned Database::getHiscore(const Song& s) const {
	const SongId songid = m_songs.lookup(s);

	if(!songid.has_value())
		return 0;
    
	return m_hiscores.getHiscore(songid.value());
}

