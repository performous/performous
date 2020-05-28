#include "requesthandler.hh"
#include "unicode.hh"
#include "fs.hh"
#include <boost/filesystem.hpp>

#ifdef USE_WEBSERVER

RequestHandler::RequestHandler(std::string url, unsigned short port, Songs& songs): m_songs(songs), m_restinio_server(restinio::own_io_context(), make_server_settings(url, port)) {
	m_local_ip = RequestHandler::getLocalIP(std::string("1.1.1.1"));
	fs::path extensionsConfig = getConfigDir() / std::string("file-extensions.json");
	if (!fs::exists(extensionsConfig)) {
		std::string message("webserver/notice: file-extensions.json not found \"" + getConfigDir().string() + "\", so we'll create it now.");
		std::clog << message << std::endl;
		try {
		fs::path originPath(getShareDir() / "config" / std::string("file-extensions.json"));
		fs::copy_file(originPath, extensionsConfig);
		} catch (std::exception const& e) {
			std::clog << "webserver/error: file-extensions.json couldn't be copied due to... " << e.what() << std::endl;
		}
	}
	m_contentTypes = readJSON(extensionsConfig);
}

std::string RequestHandler::getContentType(const std::string& extension) {
	if (m_contentTypes.contains(extension)) {
		return m_contentTypes[extension].get<std::string>();
	}
	return std::string("text/plain;charset=UTF-8");
}

RequestHandler::~RequestHandler() {
	try {
		m_restinio_server.close_sync();
		m_restinio_server.io_context().stop();
		} catch (const std::exception &e) {
		std::clog << "webserver/error: Failed to close RESTinio server due to: " << e.what() << "." << std::endl;
	}
}

boost::asio::ip::network_v4 Performous_IP_Blocker::m_allowed_subnet;

Performous_Server_Settings RequestHandler::make_server_settings(const std::string &url, unsigned short port) {
    auto settings = Performous_Server_Settings(config["webserver/threads"].i());
    settings
    .port( port )
    .address( url )
    .separate_accept_and_create_connect(true)
    .request_handler(init_webserver_router())
    .read_next_http_message_timelimit(std::chrono::seconds(config["webserver/http_timelimit"].i()))
    .write_http_response_timelimit(std::chrono::seconds(config["webserver/write_http_timelimit"].i()))
    .handle_request_timeout(std::chrono::seconds(config["webserver/timeout"].i()))
    .buffer_size(std::size_t(config["webserver/buffer_size"].i()))
    .concurrent_accepts_count(config["webserver/threads"].i())
    .max_pipelined_requests(config["webserver/request_pipeline"].i());
    settings.ip_blocker(std::make_shared<Performous_IP_Blocker>());
    return settings;
}

boost::asio::ip::address_v4 RequestHandler::getLocalIP(const std::string& service) {
		boost::asio::io_service netService;
		boost::asio::ip::udp::resolver resolver(netService);
		boost::asio::ip::resolver_base::flags flags = boost::asio::ip::resolver_base::flags::passive | boost::asio::ip::resolver_base::flags::numeric_service;
		boost::asio::ip::address_v4 address;
		boost::asio::ip::udp::resolver::iterator endpoints;
		try {
			address = boost::asio::ip::make_address_v4(service);
			flags |= boost::asio::ip::resolver_base::flags::numeric_host;
			endpoints = resolver.resolve(boost::asio::ip::udp::resolver::query(boost::asio::ip::udp::v4(), service, "80", flags));
		} catch (const std::exception& e) {
			endpoints = resolver.resolve(boost::asio::ip::udp::resolver::query(boost::asio::ip::udp::v4(), boost::asio::ip::host_name(), "80", flags));
		}
		boost::asio::ip::udp::endpoint ep = *endpoints;
		boost::asio::ip::udp::socket socket(netService);
		socket.connect(ep);
		return socket.local_endpoint().address().to_v4();
}

std::unique_ptr<Performous_Router_t> RequestHandler::init_webserver_router() {
	auto router = std::make_unique<Performous_Router_t>();
	router->http_get("/", [this](auto req, auto){
		return HandleFile(req, findFile("index.html"));
	});	
	router->http_get(R"--(/:path(.*))--", [this](auto request, auto params){
		if (!params.has("path")) {
			init_resp(request->create_response(restinio::status_bad_request()))
			.set_body("Please make a query to the API.")
			.done();
			return restinio::request_rejected();
		}
		std::string path(restinio::cast_to<std::string>(params["path"]));
        restinio::query_string_params_t query = restinio::parse_query<restinio::parse_query_traits::javascript_compatible>(request->header().query());
		if (path == "api/getDataBase.json") { // Get database
        m_songs.setFilter(std::string(), true);
        size_t sort = 1;
        bool descending = (query.has("order") && query["order"] == "descending");
        if (query.has("order")) {
        	std::string order(query["order"]);
        	if (UnicodeUtil::toLower(order) == "title") sort = 1;
        	else if (UnicodeUtil::toLower(order) == "artist") sort = 2;
        	else if (UnicodeUtil::toLower(order) == "edition") sort = 3;
        	else if (UnicodeUtil::toLower(order) == "language") sort = 6;
        	m_songs.sortSpecificChange(sort, descending);
        }
        nlohmann::json jsonRoot = SongsToJsonObject();
		init_resp(request->create_response(restinio::status_ok()),std::string("application/json"))
			.set_body(jsonRoot.dump())
			.done();
		return restinio::request_accepted();
	}
	else if (path == "api/language") {
        auto localeMap = GenerateLocaleDict();
        nlohmann::json jsonRoot = nlohmann::json();
            for (auto const &kv : localeMap) {
                std::string key = kv.first;
                //Hack to get an easy key value pair within the json object.
                if(key == "Web interface by Niek Nooijens and Arjan Speiard, for full credits regarding Performous see /docs/Authors.txt"){
                    key = "Credits";
                }
                std::replace(key.begin(), key.end(), ' ','_');
                key = UnicodeUtil::toLower(key);;
                jsonRoot[key] = kv.second;
            }
    	    init_resp(request->create_response(restinio::status_ok()),std::string("application/json"))
        		.set_body(jsonRoot.dump())
        		.done();
			return restinio::request_accepted();
        }
        else if (path == "api/getCurrentPlaylist.json") {
        Game* gm = Game::getSingletonPtr();
        nlohmann::json jsonRoot = nlohmann::json::array();
        for (auto const& song : gm->getCurrentPlayList().getList()) {
            nlohmann::json songObject;
            songObject["Title"] = song->title;
            songObject["Artist"] = song->artist;
            songObject["Edition"] = song->edition;
            songObject["Language"] = song->language;
            songObject["Creator"] = song->creator;
            songObject["Duration"] = song->getDurationSeconds();
            jsonRoot.push_back(songObject);
        }
        init_resp(request->create_response(restinio::status_ok()),std::string("application/json"))
			.set_body(jsonRoot.dump())
			.done();
		return restinio::request_accepted();
    }
    else if (path == "api/getplaylistTimeout") {
    	init_resp(request->create_response(restinio::status_ok()))
        .set_body(std::to_string(config["game/playlist_screen_timeout"].i()))
        .done();
		return restinio::request_accepted();
    }
	else {
		return HandleFile(request, findFile(std::string(params["path"])));
		}
	});
	
	/// POST Handlers
	
    router->http_post(R"--(/:path(.*))--", [this](auto request, auto params) {
    	if (!params.has("path")) {
			init_resp(request->create_response(restinio::status_bad_request()))
			.set_body("Please make a query to the API.")
			.done();
			return restinio::request_rejected();
		}
		
		std::string path(restinio::cast_to<std::string>(params["path"]));
        Game* gm = Game::getSingletonPtr();
        nlohmann::json jsonPostBody;
		try {
    		jsonPostBody = nlohmann::json::parse(request->body());
	    } catch (nlohmann::json::exception const& e) {
			std::clog << "webserver/error: JSON exception was thrown \"" << e.what() << "\"." << std::endl;
    	}
    	if (jsonPostBody.empty()) {
			init_resp(request->create_response(restinio::status_bad_request()))
			.set_body("POST Body malformed, please make a valid request.")
			.done();
			return restinio::request_rejected();
    	}
        if (path == "api/add") {
			m_songs.setFilter(std::string(), true);
			std::shared_ptr<Song> songPointer = GetSongFromJSON(jsonPostBody);
			if(!songPointer) {
				std::string temp("Song \"" + jsonPostBody["Artist"].get<std::string>() + " - " + jsonPostBody["Title"].get<std::string>() + "\" was not found.");
				init_resp(request->create_response(restinio::status_not_found()))
					.set_body(temp)
					.done();
				return restinio::request_accepted();
			} else {
				gm->getCurrentPlayList().addSong(songPointer);
				ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
				m_pp->triggerSongListUpdate();
				init_resp(request->create_response(restinio::status_ok()))
					.set_body("success")
					.done();
				return restinio::request_accepted();
			}
		}
		else if(path == "api/remove") {
			if(gm->getCurrentPlayList().isEmpty()) {
				init_resp(request->create_response(restinio::status_conflict()))
					.set_body("Playlist is empty.")
					.done();
				return restinio::request_accepted();
			}
				auto songIdToDelete = jsonPostBody["songId"].get<int>();
				if(songIdToDelete >= 0) {
					gm->getCurrentPlayList().removeSong(songIdToDelete);
					ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
					m_pp->triggerSongListUpdate();
					init_resp(request->create_response(restinio::status_ok()))
					.set_body("success")
					.done();
				return restinio::request_accepted();
				}
				else {
					std::string temp("Can't remove songs from the playlist with a negative id \"" + std::to_string(songIdToDelete) +"\". Please make a valid request.");
					init_resp(request->create_response(restinio::status_bad_request()))
						.set_body(temp)
						.done();
					return restinio::request_accepted();
				}
    	}
    	else if (path == "api/setposition") {
			if(gm->getCurrentPlayList().isEmpty()) {
				init_resp(request->create_response(restinio::status_conflict()))
					.set_body("Playlist is empty.")
					.done();
				return restinio::request_accepted();
			}
			try {
				auto songIdToMove = jsonPostBody["songId"].get<int>();
				auto positionToMoveTo = jsonPostBody["position"].get<int>();
				int sizeOfPlaylist = gm->getCurrentPlayList().getList().size();
				if(songIdToMove < 0) {
					std::string temp("Can't move songs with a negative id \"" + std::to_string(songIdToMove) + "\". Please make a valid request.");
					init_resp(request->create_response(restinio::status_bad_request()))
						.set_body(temp)
						.done();
					return restinio::request_accepted();
				}
				if(positionToMoveTo < 0) {
					std::string temp("Can't move songs to a negative position \"" + std::to_string(positionToMoveTo) + "\". Please make a valid request.");
					init_resp(request->create_response(restinio::status_bad_request()))
					.set_body(temp)
					.done();
					return restinio::request_accepted();
				}
				if(songIdToMove > sizeOfPlaylist - 1) {
					std::string temp("Song ID \"" + std::to_string(songIdToMove + 1) + "\" is beyond the playlist bounds. Please make a valid request.");
					init_resp(request->create_response(restinio::status_bad_request()))
						.set_body(temp)
						.done();
					return restinio::request_accepted();
				}
				if(positionToMoveTo <= sizeOfPlaylist - 1) {
					gm->getCurrentPlayList().setPosition(songIdToMove,positionToMoveTo);
					ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
					m_pp->triggerSongListUpdate();
					init_resp(request->create_response(restinio::status_ok()))
						.set_body("success")
						.done();
						return restinio::request_accepted();
				} else  {
					std::string temp("Not gonna move the song to \""+ std::to_string(positionToMoveTo + 1) + "\" since the list ain't that long. Please make a valid request.");
					init_resp(request->create_response(restinio::status_conflict()))
						.set_body(temp)
						.done();
					return restinio::request_accepted();
				}
			} catch(nlohmann::json::exception const & e) {
				std::string str = std::string("JSON Exception: ") + e.what();
				init_resp(request->create_response(restinio::status_bad_request()))
					.set_body(str)
					.done();
				return restinio::request_accepted();
			}
    	}
    	else if (path == "api/search") {
			auto query = jsonPostBody.find("query");
			m_songs.setFilter(*query, true);
			nlohmann::json jsonRoot = nlohmann::json::array();
			for(auto song = m_songs.begin(true); song != m_songs.end(true); song++) {
				nlohmann::json songObject;
				songObject["Title"] = song->get()->title;
				songObject["Artist"] = song->get()->artist;
				songObject["Edition"] = song->get()->edition;
				songObject["Language"] = song->get()->language;
				songObject["Creator"] = song->get()->creator;
				jsonRoot.push_back(songObject);
			}
			init_resp(request->create_response(restinio::status_ok()),std::string("application/json"))	
				.set_body(jsonRoot.dump())
				.done();
			return restinio::request_accepted();
    	}
    	else {
			init_resp(request->create_response(restinio::status_not_found()))
			.set_body("API Path not found.")
			.done();
			return restinio::request_rejected();
    	}
    });
	router->http_put(R"-(.*)-", [](auto request, auto){
        init_resp(request->create_response(restinio::status_ok()))
            .done();
		return restinio::request_accepted();
	});
	router->http_delete(R"-(.*)-", [](auto request, auto){
        init_resp(request->create_response(restinio::status_ok()))
            .done();
		return restinio::request_accepted();
	});
    router->non_matched_request_handler(
            [](auto request){
            	std::clog << "webserver/notice: Non-matched request." << std::endl;
                request->create_response(restinio::status_not_found()).done();
                return restinio::request_rejected();
            });
	return router;
}

restinio::request_handling_status_t RequestHandler::HandleFile(std::shared_ptr<restinio::request_t> request, const fs::path& filePath) {
	if (!filePath.has_filename()) {
		init_resp(request->create_response(restinio::status_forbidden()))
				.append_header_date_field()
				.set_body("Directory listings are forbidden.")
				.done();
		return restinio::request_rejected();
	}
	std::string content_type;
    fs::path fileName = filePath.filename();
    if (!fileName.has_extension()) {
    	content_type = "text/plain;charset=UTF-8";
    }
    else {
    	content_type = getContentType(fileName.extension().string());
    }
    std::string fileToSend = findFile(fileName).string();
	try {
		auto file = restinio::sendfile(fileToSend);
		auto modified_at = restinio::make_date_field_value(file.meta().last_modified_at());
		init_resp(request->create_response(restinio::status_ok()),content_type)
						.append_header_date_field()
						.append_header(restinio::http_field::last_modified, std::move(modified_at))
						.set_body(std::move(file))
						.done();
		return restinio::request_accepted();
		} catch(const std::exception &e) {
			init_resp(request->create_response(restinio::status_not_found()))
					.append_header_date_field()
					.done();
		return restinio::request_rejected();
		}
}

nlohmann::json RequestHandler::SongsToJsonObject() {
    nlohmann::json jsonRoot = nlohmann::json::array();
    for (auto const& song: m_songs) {
        nlohmann::json songObject;
        songObject["Title"] = song->title;
        songObject["Artist"] = song->artist;
        songObject["Edition"] = song->edition;
        songObject["Language"] = song->language;
        songObject["Creator"] = song->creator;
        songObject["name"] = song->artist + " " + song->title;
        jsonRoot.push_back(songObject);
    }
    return jsonRoot;
}

std::shared_ptr<Song> RequestHandler::GetSongFromJSON(nlohmann::json jsonDoc) {
    m_songs.setFilter(std::string(), true);

    for (auto const& song: m_songs) {
        if(song->title == jsonDoc["Title"] &&
           song->artist == jsonDoc["Artist"] &&
           song->edition == jsonDoc["Edition"] &&
           song->language == jsonDoc["Language"] &&
           song->creator == jsonDoc["Creator"]) {
            std::clog << "webserver/info: Found requested song." << std::endl;
            return song;
        }
    }
    std::clog << "webserver/info: Couldn't find requested song." << std::endl;
    return std::shared_ptr<Song>();
}

std::map<std::string, std::string> RequestHandler::GenerateLocaleDict() {
    std::vector<std::string> translationKeys = GetTranslationKeys();
    std::map<std::string, std::string> localeMap;
    for (auto const &translationKey : translationKeys) {
        localeMap[translationKey] = _(translationKey);
    }
    return localeMap;
}

std::vector<std::string> RequestHandler::GetTranslationKeys() {
    std::vector<std::string> tranlationKeys = { 
        translate_noop("Performous web frontend"),
        translate_noop("View database"),
        translate_noop("View playlist"),
        translate_noop("Search and Add"),
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
        translate_noop("Successfully removed song from playlist"),
        translate_noop("Failed removing song from playlist"),
        translate_noop("Successfully changed position of song"),
        translate_noop("Failed changing position of song"),
        translate_noop("Successfully moved song up"),
        translate_noop("Failed moving song up"),
        translate_noop("Successfully moved song down"),
        translate_noop("Failed moving song down"),
        translate_noop("Successfully added song to the playlist"),
        translate_noop("Failed adding song to the playlist"),
        translate_noop("No songs found with current filter")
    };

    return tranlationKeys;
}
#endif
