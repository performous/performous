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
		
		for (auto const& [song_id, song] : m_songs.getSongItems()) {
			if (song.timesPlayed < 0) {
				m_songs.addSongItem(song.artist, song.title, m_hiscores.getHiscores(song.id).size(), song.id);
			}
		}

		std::clog << "database/info: Loaded " << m_players.count() << " players, " << m_songs.size() << " songs and " << m_hiscores.size() << " hiscores from " << m_filename.string() << std::endl;
	} catch (std::exception const& e) {
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

Players const& Database::getPlayers() const {
	return m_players;
}

void Database::incrementSongPlayed(std::shared_ptr<Song> s) {
	m_songs.incrementSongPlayed(s);
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
	unsigned short level = config["game/difficulty"].ui();
	m_hiscores.addHiscore(score, playerid, s->id, level, track);
	std::clog << "database/info: Added new hiscore " << score << " points on track " << track << " of songid " << std::to_string(s->id) << " level "<< level<< std::endl;
}

bool Database::reachedHiscore(std::shared_ptr<Song> s) const {
	unsigned score = scores.front().score;
	const auto track = scores.front().track;
	unsigned short level = config["game/difficulty"].ui();
	return m_hiscores.reachedHiscore(score, s->id, level, track);
}

std::vector<HiscoreItem> Database::queryPerSongHiscore(std::shared_ptr<Song> s, std::string const& track) const {
	return m_hiscores.queryHiscore(std::nullopt, s->id, track, std::nullopt);
}

unsigned Database::resolveToSongId(Song const& s) const {
	try {
		return m_songs.resolveToSongId(s);
	}
	catch (const std::exception&) {
		return -1;
	}
}

unsigned Database::getTimesPlayed(Song const& s) const {
	try {
		return m_songs.getSongItemById(s.id)->timesPlayed;
	}
	catch (const std::exception&) {
		return 0;
	}
}

bool Database::hasHiscore(Song const& s) const {
	try {
		return m_hiscores.hasHiscore(s.id);
	} catch (const std::exception&) {
		return false;
	}
}

unsigned Database::getHiscore(Song const& s) const {
	try {
		return m_hiscores.getHiscore(s.id);
	} catch (const std::exception&) {
		return 0;
	}
}

unsigned Database::getHiscore(SongPtr const& s) const {
	try {
		return m_hiscores.getHiscore(s->id);
	} catch (const std::exception& e) {
		std::clog << "database/error: Invalid song ID for song: " + s->artist + " - " + s->title << std::endl;
		std::clog << "database/error: message: " << e.what() << std::endl;
		throw;
	}
}

std::vector<HiscoreItem> Database::getHiscores(SongPtr const& s) const {
	try {
		return m_hiscores.getHiscores((*s).id);
	} catch (const std::exception& e) {
		std::clog << "database/error: Invalid song ID for song: " + s->artist + " - " + s->title << std::endl;
		std::clog << "database/error: message: " << e.what() << std::endl;
		throw;
	}
}

