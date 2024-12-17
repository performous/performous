#include "database.hh"

#include "configuration.hh"
#include "libxml++.hh"
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
		SpdLogger::info(LogSystem::DATABASE, "Loaded {} players, {} songs, and {} hiscores from {}.", m_players.count(), m_songs.size(), m_hiscores.size(), m_filename);
	} catch (std::exception const& e) {
		SpdLogger::error(LogSystem::DATABASE, "Error loading file={}, error={}", m_filename, e.what());
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
		SpdLogger::info(LogSystem::DATABASE, "Saved {} players, {} songs, and {} hiscores to {}.", m_players.count(), m_songs.size(), m_hiscores.size(), m_filename);
	} catch (std::exception const& e) {
		SpdLogger::error(LogSystem::DATABASE, "Error saving file={}, error={}.", m_filename, e.what());
		return;
	}
}

void Database::addPlayer(std::string const& name, std::string const& picture, std::optional<PlayerId> id) {
	m_players.addPlayer(name, picture, id);
}

Players const& Database::getPlayers() const {
	return m_players;
}

void Database::addSong(std::shared_ptr<Song> s) {
	m_songs.addSong(s);
}

void Database::addHiscore(std::shared_ptr<Song> s) {
	auto maybe_playerid = m_players.lookup(m_players.current().name);
	if (!maybe_playerid.has_value()) {
		SpdLogger::error(LogSystem::DATABASE, "Cannot find player id for player={}", m_players.current().name);
		return;
	}
	auto playerid = maybe_playerid.value();
	unsigned score = scores.front().score;
	std::string track = scores.front().track;
	const auto songid = m_songs.lookup(s);
	if(!songid.has_value()) {
		SpdLogger::error(LogSystem::DATABASE, "Invalid song ID for song: artist={}, title={}", s->artist, s->title);
		return;
	}
	unsigned short level = config["game/difficulty"].ui();
	m_hiscores.addHiscore(score, playerid, songid.value(), level, track);
	SpdLogger::info(LogSystem::DATABASE, "Added new hiscore. Score={} on track={} for song id={}, on level={}", score, track, songid.value(), level);
}

bool Database::reachedHiscore(std::shared_ptr<Song> s) const {
	unsigned score = scores.front().score;
	const auto track = scores.front().track;
	const auto songid = m_songs.lookup(s);
	if (!songid) {
		SpdLogger::error(LogSystem::DATABASE, "Invalid song ID for song: artist={}, title={}", s->artist, s->title);
		return false;
	}
	unsigned short level = config["game/difficulty"].ui();
	return m_hiscores.reachedHiscore(score, songid.value(), level, track);
}

std::vector<HiscoreItem> Database::queryPerSongHiscore(std::shared_ptr<Song> s, std::string const& track) const {
	auto const maybe_songid = m_songs.lookup(s);

	if (!maybe_songid) {
		return {}; // song not yet in database.
	}

	auto const songid = maybe_songid.value();

	return m_hiscores.queryHiscore(std::nullopt, songid, track, std::nullopt);
}

bool Database::hasHiscore(Song const& s) const {
	try {
		return m_hiscores.hasHiscore(m_songs.lookup(s).value());
	} catch (const std::exception&) {
		return false;
	}
}

unsigned Database::getHiscore(Song const& s) const {
	try {
		const auto songid = m_songs.lookup(s);
		return m_hiscores.getHiscore(songid.value());
	} catch (const std::exception&) {
		return 0;
	}
}

unsigned Database::getHiscore(SongPtr const& s) const {
	try {
		auto const songid = m_songs.getSongId(s);

		return m_hiscores.getHiscore(songid);
	} catch (const std::exception& e) {
		SpdLogger::error(LogSystem::DATABASE, "Invalid song ID for song: artist={}, title={}, error={}", s->artist, s->title, e.what());
		throw;
	}
}

std::vector<HiscoreItem> Database::getHiscores(SongPtr const& s) const {
	try {
		auto const songid = m_songs.getSongId(s);

		return m_hiscores.getHiscores(songid);
	} catch (const std::exception& e) {
		SpdLogger::error(LogSystem::DATABASE, "Invalid song ID for song: artist={}, title={}, error={}", s->artist, s->title, e.what());
		throw;
	}
}

