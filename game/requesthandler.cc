#include "requesthandler.hh"

#ifdef USE_WEBSERVER

#include "game.hh"
#include "fs.hh"
#include "image.hh"
#include "log.hh"
#include "screen_playlist.hh"
#include "unicode.hh"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

// Minimal MIME lookup for the few static asset types the web frontend uses.
// Falls back to image detection for images, octet-stream otherwise.
std::string mimeTypeFor(const std::string& path) {
	if (path.find(".html") != std::string::npos) return "text/html";
	if (path.find(".js") != std::string::npos) return "text/javascript";
	if (path.find(".css") != std::string::npos) return "text/css";
	const std::string& imageMime = getImageMimeType(path);
	if (imageMime != "application/octet-stream") return imageMime;
	return "application/octet-stream";
}

void replyJson(httplib::Response& res, const nlohmann::json& value) {
	// set_content sets Content-Type; assigning res.content_type directly is
	// only available in newer cpp-httplib versions.
	res.set_content(value.dump(), "application/json");
}

} // namespace

RequestHandler::RequestHandler(Game& game, const std::string& host, int port, Songs& songs)
	: m_server(std::make_unique<httplib::Server>()), m_game(game), m_songs(songs) {
	setupRoutes();
	// listen() is bound later with host/port so construction never fails.
	m_bindHost = host;
	m_bindPort = port;
}

void RequestHandler::setupRoutes() {
	// API routes first; a catch-all static file handler is registered last.

	m_server->Get("/api/getDataBase.json", [this](const httplib::Request& req, httplib::Response& res) {
		handleGetDatabase(req, res);
	});

	m_server->Get("/api/language", [this](const httplib::Request& req, httplib::Response& res) {
		handleLanguage(req, res);
	});

	m_server->Get("/api/getCurrentPlaylist.json", [this](const httplib::Request& req, httplib::Response& res) {
		handleGetCurrentPlaylist(req, res);
	});

	m_server->Get("/api/getplaylistTimeout", [this](const httplib::Request& req, httplib::Response& res) {
		handlePlaylistTimeout(req, res);
	});

	m_server->Post("/api/add", [this](const httplib::Request& req, httplib::Response& res) {
		handleAdd(req, res);
	});

	m_server->Post("/api/remove", [this](const httplib::Request& req, httplib::Response& res) {
		handleRemove(req, res);
	});

	m_server->Post("/api/setposition", [this](const httplib::Request& req, httplib::Response& res) {
		handleSetPosition(req, res);
	});

	m_server->Post("/api/search", [this](const httplib::Request& req, httplib::Response& res) {
		handleSearch(req, res);
	});

	// PUT/DELETE kept as no-op success stubs, matching the previous behaviour.
	m_server->Put(R"(/.*)", [](const httplib::Request&, httplib::Response& res) {
		res.status = 200;
	});
	m_server->Delete(R"(/.*)", [](const httplib::Request&, httplib::Response& res) {
		res.status = 200;
	});

	// Static files catch-all (registered last so API routes win).
	m_server->Get(R"(/.*)", [this](const httplib::Request& req, httplib::Response& res) {
		const std::string path = req.path;
		std::string fileName;
		if (path == "/" || path.empty()) {
			fileName = "index.html";
		} else {
			fileName = fs::path(path).filename().string();
		}
		const std::string clientIp = req.remote_addr;
		fs::path fileToSend;
		try {
			fileToSend = findFile(fileName);
		} catch (const std::runtime_error& e) {
			SpdLogger::error(LogSystem::WEBSERVER, "HandleFile() File Not Found. Client {}. {}", clientIp, e.what());
			res.status = 404;
			res.set_content(std::string("INTERNAL ERROR, MISSING FILE: ") + e.what(), "text/plain");
			return;
		}
		std::ifstream ifs(fileToSend, std::ios::binary);
		if (!ifs) {
			SpdLogger::error(LogSystem::WEBSERVER, "HandleFile() Unable to read file. Client {} {}", clientIp, fileToSend.string());
			res.status = 500;
			res.set_content("INTERNAL ERROR: Unable to read file", "text/plain");
			return;
		}
		std::ostringstream ss;
		ss << ifs.rdbuf();
		res.set_content(ss.str(), mimeTypeFor(fileName).c_str());
	});
}

void RequestHandler::listen() {
	// .c_str() for compatibility with older cpp-httplib (e.g. Ubuntu 22.04's
	// 0.10.x), whose listen() only accepts const char*, not std::string.
	if (!m_server->listen(m_bindHost.c_str(), m_bindPort)) {
		throw std::runtime_error("Failed to bind webserver on " + m_bindHost + ":" + std::to_string(m_bindPort));
	}
}

void RequestHandler::stop() {
	if (m_server) m_server->stop();
}

bool RequestHandler::is_running() const {
	return m_server && m_server->is_running();
}

void RequestHandler::handleGetDatabase(const httplib::Request& req, httplib::Response& res) {
	// Compare parsed params individually so ordering and httplib version
	// differences (raw query string vs parsed params) cannot break sorting.
	auto param = [&req](const std::string& key) -> std::string {
		auto it = req.params.find(key);
		return it != req.params.end() ? it->second : std::string();
	};
	const std::string sort = param("sort");
	const std::string order = param("order");
	const bool descending = (order == "descending");
	m_songs.setFilter("");
	if (sort == "artist") {
		m_songs.sortSpecificChange(2, descending);
	} else if (sort == "title") {
		m_songs.sortSpecificChange(1, descending);
	} else if (sort == "language") {
		m_songs.sortSpecificChange(6, descending);
	} else if (sort == "edition") {
		m_songs.sortSpecificChange(3, descending);
	} else if (sort == "creator") {
		m_songs.sortSpecificChange(10, descending);
	}
	// make sure to apply the filtering
	m_songs.update();
	replyJson(res, songsToJson());
}

void RequestHandler::handleLanguage(const httplib::Request&, httplib::Response& res) {
	auto localeMap = generateLocaleDict();
	nlohmann::json jsonRoot = nlohmann::json::object();
	for (auto const& kv : localeMap) {
		std::string key = kv.first;
		// Hack to get an easy key value pair within the json object.
		if (key == "Web interface by Niek Nooijens and Arjan Speiard, for full credits regarding Performous see /docs/Authors.txt") {
			key = "Credits";
		}
		std::replace(key.begin(), key.end(), ' ', '_');
		key = UnicodeUtil::toLower(key);
		jsonRoot[key] = kv.second;
	}
	replyJson(res, jsonRoot);
}

void RequestHandler::handleGetCurrentPlaylist(const httplib::Request&, httplib::Response& res) {
	nlohmann::json jsonRoot = nlohmann::json::array();
	for (auto const& song : m_game.getCurrentPlayList().getList()) {
		nlohmann::json songObject;
		songObject["Title"] = song->title;
		songObject["Artist"] = song->artist;
		songObject["Edition"] = song->edition;
		songObject["Language"] = song->language;
		songObject["Creator"] = song->creator;
		songObject["Duration"] = song->getDurationSeconds();
		songObject["HasError"] = song->loadStatus == Song::LoadStatus::PARSERERROR;
		songObject["ProvidedBy"] = song->providedBy;
		songObject["Comment"] = song->comment;
		jsonRoot.push_back(std::move(songObject));
	}
	replyJson(res, jsonRoot);
}

void RequestHandler::handlePlaylistTimeout(const httplib::Request&, httplib::Response& res) {
	res.set_content(std::to_string(config["game/playlist_screen_timeout"].ui()), "text/plain");
}

void RequestHandler::handleAdd(const httplib::Request& req, httplib::Response& res) {
	nlohmann::json jsonPostBody = extractJson(req);
	if (jsonPostBody.is_null()) {
		res.status = 400;
		res.set_content("Post body is malformed. Please make a valid request.", "text/plain");
		return;
	}
	std::shared_ptr<Song> songPointer = getSongFromJson(jsonPostBody);
	if (!songPointer) {
		res.status = 404;
		res.set_content("Song \"" + jsonPostBody.value("Artist", "") + " - " + jsonPostBody.value("Title", "") + "\" was not found.", "text/plain");
		return;
	} else if (songPointer->loadStatus == Song::LoadStatus::PARSERERROR) {
		res.status = 404;
		res.set_content("Song \"" + jsonPostBody.value("Artist", "") + " - " + jsonPostBody.value("Title", "") + "\" Song load status is error. Please check what's wrong with it.", "text/plain");
		return;
	}
	SpdLogger::debug(LogSystem::WEBSERVER, "Adding {} - {} to the playlist.", songPointer->artist, songPointer->title);
	m_game.getCurrentPlayList().addSong(songPointer);
	ScreenPlaylist* pp = dynamic_cast<ScreenPlaylist*>(m_game.getScreen("Playlist"));
	pp->triggerSongListUpdate();
	res.set_content("success", "text/plain");
}

void RequestHandler::handleRemove(const httplib::Request& req, httplib::Response& res) {
	if (m_game.getCurrentPlayList().isEmpty()) {
		res.status = 400;
		res.set_content("Playlist is empty.", "text/plain");
		return;
	}
	nlohmann::json jsonPostBody = extractJson(req);
	if (jsonPostBody.is_null() || !jsonPostBody.contains("songId") || !jsonPostBody["songId"].is_number_unsigned()) {
		res.status = 400;
		res.set_content("JSON Exception: songId is not a valid number", "text/plain");
		return;
	}
	unsigned songIdToDelete = jsonPostBody["songId"].get<unsigned>();
	m_game.getCurrentPlayList().removeSong(songIdToDelete);
	ScreenPlaylist* pp = dynamic_cast<ScreenPlaylist*>(m_game.getScreen("Playlist"));
	pp->triggerSongListUpdate();
	res.set_content("success", "text/plain");
}

void RequestHandler::handleSetPosition(const httplib::Request& req, httplib::Response& res) {
	if (m_game.getCurrentPlayList().isEmpty()) {
		res.status = 400;
		res.set_content("Playlist is empty.", "text/plain");
		return;
	}
	nlohmann::json jsonPostBody = extractJson(req);
	if (jsonPostBody.is_null() || !jsonPostBody.contains("songId") || !jsonPostBody["songId"].is_number_unsigned()
		|| !jsonPostBody.contains("position") || !jsonPostBody["position"].is_number_unsigned()) {
		res.status = 400;
		res.set_content("JSON Exception: songId/position is not a valid number", "text/plain");
		return;
	}
	unsigned songIdToMove = jsonPostBody["songId"].get<unsigned>();
	unsigned positionToMoveTo = jsonPostBody["position"].get<unsigned>();
	unsigned sizeOfPlaylist = static_cast<unsigned>(m_game.getCurrentPlayList().getList().size());
	if (songIdToMove > sizeOfPlaylist - 1) {
		res.status = 400;
		res.set_content("Not gonna move the unknown song you've provided \"" + std::to_string(songIdToMove + 1) + "\". Please make a valid request.", "text/plain");
		return;
	}
	if (positionToMoveTo <= sizeOfPlaylist - 1) {
		m_game.getCurrentPlayList().move(songIdToMove, positionToMoveTo);
		ScreenPlaylist* pp = dynamic_cast<ScreenPlaylist*>(m_game.getScreen("Playlist"));
		pp->triggerSongListUpdate();
		res.set_content("success", "text/plain");
		return;
	}
	res.status = 400;
	res.set_content("Not gonna move the song to \"" + std::to_string(positionToMoveTo + 1) + "\" since the list ain't that long. Please make a valid request.", "text/plain");
}

void RequestHandler::handleSearch(const httplib::Request& req, httplib::Response& res) {
	nlohmann::json jsonPostBody = extractJson(req);
	if (jsonPostBody.is_null() || !jsonPostBody.contains("query") || !jsonPostBody["query"].is_string()) {
		res.status = 400;
		res.set_content("Post body is malformed. Please make a valid request.", "text/plain");
		return;
	}
	const std::string query = jsonPostBody["query"].get<std::string>();
	m_songs.setFilter(query);
	nlohmann::json jsonRoot = nlohmann::json::array();
	for (size_t i = 0; i < m_songs.size(); i++) {
		nlohmann::json songObject;
		songObject["Title"] = m_songs[i]->title;
		songObject["Artist"] = m_songs[i]->artist;
		songObject["Edition"] = m_songs[i]->edition;
		songObject["Language"] = m_songs[i]->language;
		songObject["Creator"] = m_songs[i]->creator;
		songObject["HasError"] = m_songs[i]->loadStatus == Song::LoadStatus::PARSERERROR;
		songObject["ProvidedBy"] = m_songs[i]->providedBy;
		songObject["Comment"] = m_songs[i]->comment;
		jsonRoot.push_back(std::move(songObject));
	}
	replyJson(res, jsonRoot);
}

nlohmann::json RequestHandler::extractJson(const httplib::Request& req) {
	if (req.body.empty()) return nlohmann::json();
	nlohmann::json jsonBody = nlohmann::json::parse(req.body, nullptr, false);
	if (jsonBody.is_discarded()) {
		SpdLogger::error(LogSystem::WEBSERVER, "JSON parse ERROR on request body.");
		return nlohmann::json();
	}
	return jsonBody;
}

nlohmann::json RequestHandler::songsToJson() const {
	nlohmann::json jsonRoot = nlohmann::json::array();
	for (size_t i = 0; i < m_songs.size(); i++) {
		nlohmann::json songObject;
		songObject["Title"] = m_songs[i]->title;
		songObject["Artist"] = m_songs[i]->artist;
		songObject["Edition"] = m_songs[i]->edition;
		songObject["Language"] = m_songs[i]->language;
		songObject["Creator"] = m_songs[i]->creator;
		songObject["name"] = m_songs[i]->artist + " " + m_songs[i]->title;
		songObject["HasError"] = m_songs[i]->loadStatus == Song::LoadStatus::PARSERERROR;
		songObject["ProvidedBy"] = m_songs[i]->providedBy;
		songObject["Comment"] = m_songs[i]->comment;
		jsonRoot.push_back(std::move(songObject));
	}
	return jsonRoot;
}

std::shared_ptr<Song> RequestHandler::getSongFromJson(const nlohmann::json& doc) const {
	m_songs.setFilter("");
	auto get = [&doc](const char* key) -> std::string {
		if (doc.contains(key) && doc[key].is_string()) return doc[key].get<std::string>();
		return std::string();
	};
	const std::string title = get("Title");
	const std::string artist = get("Artist");
	const std::string edition = get("Edition");
	const std::string language = get("Language");
	const std::string creator = get("Creator");
	const std::string providedBy = get("ProvidedBy");
	const std::string comment = get("Comment");

	for (size_t i = 0; i < m_songs.size(); i++) {
		if (m_songs[i]->title == title &&
			m_songs[i]->artist == artist &&
			m_songs[i]->edition == edition &&
			m_songs[i]->language == language &&
			m_songs[i]->creator == creator &&
			m_songs[i]->providedBy == providedBy &&
			m_songs[i]->comment == comment) {
			SpdLogger::info(LogSystem::WEBSERVER, "Found requested song, {} - {}", m_songs[i]->artist, m_songs[i]->title);
			return m_songs[i];
		}
	}
	SpdLogger::info(LogSystem::WEBSERVER, "Couldn't find requested song, {} - {}", artist, title);
	return std::shared_ptr<Song>();
}

std::map<std::string, std::string> RequestHandler::generateLocaleDict() {
	std::vector<std::string> translationKeys = getTranslationKeys();
	std::map<std::string, std::string> localeMap;
	for (auto const& translationKey : translationKeys) {
		localeMap[translationKey] = _(translationKey);
	}
	return localeMap;
}

std::vector<std::string> RequestHandler::getTranslationKeys() {
	std::vector<std::string> translationKeys = {
		translate_noop("Performous web frontend"),
		translate_noop("View database"),
		translate_noop("View playlist"),
		translate_noop("Search and add"),
		translate_noop("Sort by"),
		translate_noop("Artist"),
		translate_noop("Title"),
		translate_noop("Language"),
		translate_noop("Edition"),
		translate_noop("Creator"),
		translate_noop("Sort order"),
		translate_noop("Normal"),
		translate_noop("Inverted"),
		translate_noop("Update every 10 sec"),
		translate_noop("Refresh database"),
		translate_noop("Upcoming songs"),
		translate_noop("Refresh playlist"),
		translate_noop("Web interface by Niek Nooijens and Arjan Speiard, for full credits regarding Performous see /docs/Authors.txt"),
		translate_noop("Search"),
		translate_noop("Available songs"),
		translate_noop("Search for songs"),
		translate_noop("Yes"),
		translate_noop("No"),
		translate_noop("Move up"),
		translate_noop("Move down"),
		translate_noop("Set position"),
		translate_noop("Remove song"),
		translate_noop("Desired position of song"),
		translate_noop("Cancel"),
		translate_noop("Successfully removed song from playlist."),
		translate_noop("Failed removing song from playlist!"),
		translate_noop("Successfully changed position of song."),
		translate_noop("Failed changing position of song!"),
		translate_noop("Successfully moved song up."),
		translate_noop("Failed moving song up!"),
		translate_noop("Successfully moved song down."),
		translate_noop("Failed moving song down!"),
		translate_noop("Successfully added song to the playlist."),
		translate_noop("Failed adding song to the playlist!"),
		translate_noop("No songs found with current filter."),
		translate_noop("Has error")
	};

	return translationKeys;
}
#endif
