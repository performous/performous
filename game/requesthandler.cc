#include "requesthandler.hh"
#include "unicode.hh"
#include "game.hh"
#include "screen_playlist.hh"

RequestHandler::RequestHandler(std::string, Songs &songs) : m_songs(songs) {
	// register pages

	// get all static pages from the www folder
	m_server.set_base_dir(findFile("index.html").parent_path().string());

	// GET API requests
	m_server.Get("/api/language", [&](const httplib::Request&, httplib::Response& res) {
		std::map<std::string, std::string> localeMap;
		for (auto const &translationKey : GetTranslationKeys()) {
		    localeMap[translationKey] = _(translationKey);
		}
		auto jsonRoot = nlohmann::json::object();
		for (auto const &kv : localeMap) {
		    std::string key = kv.first;
		    //Hack to get an easy key value pair within the json object.
		    if(key == "Web interface by Niek Nooijens and Arjan Speiard, for full credits regarding Performous see /docs/Authors.txt"){
		        key = "Credits";
		    }
		    std::replace(key.begin(), key.end(), ' ','_');
		    jsonRoot[UnicodeUtil::toLower(key)] = kv.second;
		}
		res.set_content(jsonRoot.dump(), "application/json");
	});
	m_server.Get("/api/getDataBase.json", [&](const httplib::Request &req, httplib::Response& res) {
		m_songs.setFilter("");
		bool sortDescending = false;
		if (req.has_param("order") && req.get_param_value("order") == "descending") {
			sortDescending = true;
		}
		const std::string sortItem = (req.has_param("sort")) ? req.get_param_value("sort") : "artist";
		if(sortItem == "artist") {
		    m_songs.sortSpecificChange(2, sortDescending);
		} else if(sortItem == "title") {
		    m_songs.sortSpecificChange(1, sortDescending);
		} else if(sortItem == "language") {
		    m_songs.sortSpecificChange(6, sortDescending);
		} else if(sortItem == "edition") {
		    m_songs.sortSpecificChange(3, sortDescending);
		}
		auto jsonRoot = nlohmann::json::array();
		for (size_t i=0; i< m_songs.size(); i++) {
			jsonRoot.push_back({
				{"Title", m_songs[i]->title},
				{"Artist", m_songs[i]->artist},
				{"Edition", m_songs[i]->edition},
				{"Language", m_songs[i]->language},
				{"Creator", m_songs[i]->creator},
				{"name", m_songs[i]->artist + " " + m_songs[i]->title},
			});
		}
		res.set_content(jsonRoot.dump(), "application/json");
	});
	m_server.Get("/api/getCurrentPlaylist.json", [&](const httplib::Request &, httplib::Response& res) {
		Game* gm = Game::getSingletonPtr();
		auto jsonRoot = nlohmann::json::array();
		for (auto const& song : gm->getCurrentPlayList().getList()) {
			jsonRoot.push_back({
				{"Title", song->title},
				{"Artist", song->artist},
				{"Edition", song->edition},
				{"Language", song->language},
				{"Creator", song->creator},
				{"name", song->artist + " " + song->title},
				{"Duration", song->getDurationSeconds()},
			});
		}
		res.set_content(jsonRoot.dump(), "application/json");
	});


	m_server.Get("/api/getplaylistTimeout", [&](const httplib::Request &, httplib::Response& res) {
		res.set_content(std::to_string(config["game/playlist_screen_timeout"].i()), "text/plain");
	});

	m_server.Post("/api/add", [&](const httplib::Request &, httplib::Response& res, const httplib::ContentReader &contentReader) {
		std::string buffer;
		contentReader([&](const char *data, size_t dataLength) {
			buffer.append(data, dataLength);
			return true;
		});
		const auto jsonRoot = nlohmann::json::parse(buffer);
		for(int i = 0; i< m_songs.size(); i++) {
			if(m_songs[i]->title == jsonRoot["Title"] &&
				m_songs[i]->artist == jsonRoot["Artist"] &&
				m_songs[i]->edition == jsonRoot["Edition"] &&
				m_songs[i]->language == jsonRoot["Language"] &&
				m_songs[i]->creator == jsonRoot["Creator"]) {
				std::clog << "webserver/info: Found requested song." << std::endl;
				Game* gm = Game::getSingletonPtr();
				gm->getCurrentPlayList().addSong(m_songs[i]);
				ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
				m_pp->triggerSongListUpdate();
				res.set_content("success", "text/plain");
				return;
			}
		}
		res.status = 404;
		res.set_content("cannot find the selected song", "text/plain");
	});
	m_server.Post("/api/remove", [&](const httplib::Request &, httplib::Response& res, const httplib::ContentReader &contentReader) {
		std::string buffer;
		contentReader([&](const char *data, size_t dataLength) {
			buffer.append(data, dataLength);
			return true;
		});
		const auto jsonRoot = nlohmann::json::parse(buffer);
		const int songId = jsonRoot["songId"];
		if(songId < 0) {
			res.status = 404;
			res.set_content("cannot find the selected song", "text/plain");
			return;
		}
		Game* gm = Game::getSingletonPtr();
		if(gm->getCurrentPlayList().isEmpty()) {
			res.status = 404;
			res.set_content("playlist is empty", "text/plain");
			return;
		}
                gm->getCurrentPlayList().removeSong(songId);
                ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
                m_pp->triggerSongListUpdate();
		res.set_content("success", "text/plain");
	});
	m_server.Post("/api/setposition", [&](const httplib::Request &, httplib::Response& res, const httplib::ContentReader &contentReader) {
		std::string buffer;
		contentReader([&](const char *data, size_t dataLength) {
			buffer.append(data, dataLength);
			return true;
		});
		const auto jsonRoot = nlohmann::json::parse(buffer);
		const int songId = jsonRoot["songId"];
		if(songId < 0) {
			res.status = 404;
			res.set_content("cannot find the selected song", "text/plain");
			return;
		}
		const int newPosition = jsonRoot["position"];
		if(newPosition < 0) {
			res.status = 404;
			res.set_content("cannot find the selected position", "text/plain");
			return;
		}
		Game* gm = Game::getSingletonPtr();
		if(gm->getCurrentPlayList().isEmpty()) {
			res.status = 404;
			res.set_content("playlist is empty", "text/plain");
			return;
		}
		const int playlistSize = gm->getCurrentPlayList().getList().size();
		if(newPosition > playlistSize - 1) {
			res.status = 404;
			res.set_content("new position is out of bound", "text/plain");
			return;
		}
		gm->getCurrentPlayList().setPosition(songId, newPosition);
                ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
                m_pp->triggerSongListUpdate();
		res.set_content("success", "text/plain");
	});
	m_server.Post("/api/search", [&](const httplib::Request &, httplib::Response& res, const httplib::ContentReader &contentReader) {
		std::string buffer;
		contentReader([&](const char *data, size_t dataLength) {
			buffer.append(data, dataLength);
			return true;
		});
		const auto jsonRootReq = nlohmann::json::parse(buffer);
		const std::string query = jsonRootReq["query"];
		m_songs.setFilter(query);
		auto jsonRoot = nlohmann::json::array();
		for (size_t i=0; i< m_songs.size(); i++) {
			jsonRoot.push_back({
				{"Title", m_songs[i]->title},
				{"Artist", m_songs[i]->artist},
				{"Edition", m_songs[i]->edition},
				{"Language", m_songs[i]->language},
				{"Creator", m_songs[i]->creator},
				{"name", m_songs[i]->artist + " " + m_songs[i]->title},
			});
		}
		res.set_content(jsonRoot.dump(), "application/json");
	});
}

const std::vector<std::string> tranlationKeys = {
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
    translate_noop("No songs found with current filter.")
};

const std::vector<std::string>& RequestHandler::GetTranslationKeys() const {
    return tranlationKeys;
}
