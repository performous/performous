#include "requesthandler.hh"
#include "unicode.hh"
#include "fs.hh"
#include <boost/filesystem.hpp>

#ifdef USE_WEBSERVER

RequestHandler::RequestHandler(std::string url, unsigned short port, Songs& songs):m_listener("http://" + url), m_songs(songs), m_restinio_server(restinio::own_io_context(), make_server_settings(url, port)) {
	m_local_ip = RequestHandler::getLocalIP(std::string("1.1.1.1"));
    m_listener.support(web::http::methods::GET, std::bind(&RequestHandler::Get, this, std::placeholders::_1));
    m_listener.support(web::http::methods::PUT, std::bind(&RequestHandler::Put, this, std::placeholders::_1));
    m_listener.support(web::http::methods::POST, std::bind(&RequestHandler::Post, this, std::placeholders::_1));
    m_listener.support(web::http::methods::DEL, std::bind(&RequestHandler::Delete, this, std::placeholders::_1));
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

RequestHandler::~RequestHandler() {}

void RequestHandler::Error(pplx::task<void>& t)
{
    try
    {
        t.get();
    }
    catch(...)
    {
    }
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
	
    router->non_matched_request_handler(
            [](auto req){
                return req->create_response(restinio::status_not_found()).connection_close().done();
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

void RequestHandler::HandleFile(web::http::http_request request, std::string filePath) {
    auto path = filePath != "" ? filePath : request.relative_uri().path();
    auto fileName = path.substr(path.find_last_of("/\\") + 1);
        
    std::string fileToSend = findFile(fileName).string();

    concurrency::streams::fstream::open_istream(U(fileToSend), std::ios::in).then([=](concurrency::streams::istream is) {
        std::string content_type = std::string();
        if(path.find(".js") != std::string::npos) {
            content_type = "text/javascript";
        } else if (path.find(".css") != std::string::npos) {
            content_type = "text/css";
        } else if (path.find(".png") != std::string::npos) {
            content_type = "image/png";
        } else if (path.find(".gif") != std::string::npos) {
            content_type = "image/gif";
        } else if (path.find(".ico") != std::string::npos) {
            content_type = "image/x-icon";
        } else if (path.find(".svg") != std::string::npos) {
            content_type = "image/svg+xml";
        } else if(path.find(".htm") != std::string::npos) {
            content_type = "text/html";
        } else {
            content_type = "text/plain";
        }

        request.reply(web::http::status_codes::OK, is, U(content_type)).then([](pplx::task<void> t) {
            try {
                t.get();
            } catch(...){
                //
            }
        });

    }).then([=](pplx::task<void>t) {
        try {
            t.get();
        } catch(...) {
            request.reply(web::http::status_codes::InternalError,U("INTERNAL ERROR "));
        }
    });
}

void RequestHandler::Get(web::http::http_request request)
{
    std::string content_type = "text/html";
    auto uri = request.relative_uri().path();
    if(request.relative_uri().query() != "") {
        uri += "?" + request.relative_uri().query();
    }
    std::clog << "requesthandler/debug: path is: " << uri << std::endl;
    auto path = request.relative_uri().path();
    if (path == "/") {
        HandleFile(request, findFile("index.html").string());
    } else if (path == "/api/getDataBase.json") { //get database
        m_songs.setFilter("");
        if(request.relative_uri().query() == "sort=artist&order=ascending") {
            m_songs.sortSpecificChange(2);    
        } else if(request.relative_uri().query() == "sort=artist&order=descending") {
            m_songs.sortSpecificChange(2, true);
        } else if(request.relative_uri().query() == "sort=title&order=ascending") {
            m_songs.sortSpecificChange(1);    
        } else if(request.relative_uri().query() == "sort=title&order=descending") {
            m_songs.sortSpecificChange(1, true);
        } else if(request.relative_uri().query() == "sort=language&order=ascending") {
            m_songs.sortSpecificChange(6);    
        } else if(request.relative_uri().query() == "sort=language&order=descending") {
            m_songs.sortSpecificChange(6, true);
        } else if(request.relative_uri().query() == "sort=edition&order=ascending") {
            m_songs.sortSpecificChange(3);    
        } else if(request.relative_uri().query() == "sort=edition&order=descending") {
            m_songs.sortSpecificChange(3, true);
        }
//         web::json::value jsonRoot = SongsToJsonObject();
        nlohmann::json jsonRoot_new = SongsToJsonObject_New();
        request.reply(web::http::status_codes::OK, convertToCppRest(jsonRoot_new));
        return;
    }  else if(path == "/api/language") {
        auto localeMap = GenerateLocaleDict();
        nlohmann::json jsonRoot = nlohmann::json();
//         web::json::value jsonRoot = web::json::value::object();
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
        request.reply(web::http::status_codes::OK, convertToCppRest(jsonRoot));
        return;
    } else if(path == "/api/getCurrentPlaylist.json") {
        Game* gm = Game::getSingletonPtr();
        nlohmann::json jsonRoot = nlohmann::json::array();
//         web::json::value jsonRoot = web::json::value::array();
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

        request.reply(web::http::status_codes::OK, convertToCppRest(jsonRoot));
        return;
    } else if(path == "/api/getplaylistTimeout") {
        request.reply(web::http::status_codes::OK, U(config["game/playlist_screen_timeout"].i()));
        return;
    } else {
        HandleFile(request);
    }
};

void RequestHandler::Post(web::http::http_request request)
{
    Game* gm = Game::getSingletonPtr();

    auto uri = request.relative_uri().path();
    if(request.relative_uri().query() != "") {
        uri += "?" + request.relative_uri().query();
    }
    std::clog << "requesthandler/debug: path is: " << uri << std::endl;

    auto path = request.relative_uri().path();

    nlohmann::json jsonPostBody = ExtractJsonFromRequest_New(request);
//     web::json::value jsonPostBody = ExtractJsonFromRequest(request);

    if(jsonPostBody.is_null()) {
        request.reply(web::http::status_codes::BadRequest, "Post body is malformed. Please make a valid request.");
        return;
    }

    if (path == "/api/add") {
        m_songs.setFilter("");
        std::shared_ptr<Song> songPointer = GetSongFromJSON_New(jsonPostBody);
        if(!songPointer) {
            std::string temp("Song \"" + jsonPostBody["Artist"].get<std::string>() + " - " + jsonPostBody["Title"].get<std::string>() + "\" was not found.");
            web::json::value reply =  web::json::value::string(U(temp));
            request.reply(web::http::status_codes::NotFound, reply);
            return;
        } else {
            std::clog << "requesthandler/debug: Adding " << songPointer->artist << " - " << songPointer->title << " to the playlist " << std::endl;
            gm->getCurrentPlayList().addSong(songPointer);
            ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
            m_pp->triggerSongListUpdate();

            request.reply(web::http::status_codes::OK, "success");
            return;
        }
    } else if(path == "/api/remove") {
        if(gm->getCurrentPlayList().isEmpty()) {
            request.reply(web::http::status_codes::BadRequest, "Playlist is empty.");
            return;
        }
        try {
            auto songIdToDelete = jsonPostBody["songId"].get<int>();
            if(songIdToDelete >= 0) {
                gm->getCurrentPlayList().removeSong(songIdToDelete);
                ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
                m_pp->triggerSongListUpdate();

                request.reply(web::http::status_codes::OK, "success");
                return;
            } else {
            std::string temp("Can't remove songs from the playlist with a negative id \"" + std::to_string(songIdToDelete) +"\". Please make a valid request.");
            web::json::value reply =  web::json::value::string(U(temp));
            request.reply(web::http::status_codes::BadRequest, reply);

            request.reply(web::http::status_codes::NotFound, reply);
                return;
            }
        } catch(web::json::json_exception const & e) {
            std::string str = std::string("JSON Exception: ") + e.what();
            request.reply(web::http::status_codes::BadRequest, str);
            return;
        }
    } else if(path == "/api/setposition") {
        if(gm->getCurrentPlayList().isEmpty()) {
            request.reply(web::http::status_codes::BadRequest, "Playlist is empty.");
            return;
        }
        try {
            auto songIdToMove = jsonPostBody["songId"].get<int>();
            auto positionToMoveTo = jsonPostBody["position"].get<int>();
            int sizeOfPlaylist = gm->getCurrentPlayList().getList().size();
            if(songIdToMove < 0) {
            std::string temp("Can't move songs with a negative id \"" + std::to_string(songIdToMove) + "\". Please make a valid request.");
            web::json::value reply =  web::json::value::string(U(temp));
                request.reply(web::http::status_codes::BadRequest, reply);
                return;
            }
            if(positionToMoveTo < 0) {
                std::string temp("Can't move songs to a negative position \"" + std::to_string(positionToMoveTo) + "\". Please make a valid request.");
                web::json::value reply =  web::json::value::string(U(temp));
                request.reply(web::http::status_codes::BadRequest, reply);
                return;
            }
            if(songIdToMove > sizeOfPlaylist - 1) {
                std::string temp("Not gonna move the unknown song you've provided \"" + std::to_string(songIdToMove + 1) + "\". Please make a valid request.");
                web::json::value reply =  web::json::value::string(U(temp));
                request.reply(web::http::status_codes::BadRequest, reply);
                return;
            }
            if(positionToMoveTo <= sizeOfPlaylist - 1) {
                gm->getCurrentPlayList().setPosition(songIdToMove,positionToMoveTo);
                ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
                m_pp->triggerSongListUpdate();
                request.reply(web::http::status_codes::OK, "success");
                return;
            } else  {
                std::string temp("Not gonna move the song to \""+ std::to_string(positionToMoveTo + 1) + "\" since the list ain't that long. Please make a valid request.");
                web::json::value reply =  web::json::value::string(U(temp));
                request.reply(web::http::status_codes::BadRequest, reply);
                return;
            }
        } catch(web::json::json_exception const & e) {
            std::string str = std::string("JSON Exception: ") + e.what();
            request.reply(web::http::status_codes::BadRequest, str);
            return;
        }     
    } else if(path == "/api/search") {
        auto query = jsonPostBody["query"].get<std::string>();
        m_songs.setFilter(query);
        nlohmann::json jsonRoot = nlohmann::json::array();
//         web::json::value jsonRoot = web::json::value::array();
        for(auto const& song: m_songs) {
            nlohmann::json songObject;
            songObject["Title"] = song->title;
            songObject["Artist"] = song->artist;
            songObject["Edition"] = song->edition;
            songObject["Language"] = song->language;
            songObject["Creator"] = song->creator;
            jsonRoot.push_back(songObject);
        }
        request.reply(web::http::status_codes::OK, convertToCppRest(jsonRoot));
        return;
    } else {
        request.reply(web::http::status_codes::NotFound, "The path \""+ path +"\" was not found.");
        return;
    }
};

void RequestHandler::Delete(web::http::http_request request)
{
    request.reply(web::http::status_codes::OK);
};

void RequestHandler::Put(web::http::http_request request)
{
    request.reply(web::http::status_codes::OK);
};

web::json::value RequestHandler::ExtractJsonFromRequest(web::http::http_request request) {
    web::json::value jsonBody = web::json::value::null();
    request.extract_json().then([&jsonBody](pplx::task<web::json::value> task)
    {
         try
         {
            jsonBody = task.get();
         }
         catch (web::json::json_exception const & e)
         {
            std::clog << "webserver/error: JSON exception was thrown \"" << e.what() << "\"." << std::endl;
         }
    }).wait();

    return jsonBody;
}

nlohmann::json RequestHandler::convertFromCppRest(web::json::value const& jsonDoc) {
    utility::stringstream_t stream;
    jsonDoc.serialize(stream);
    return nlohmann::json::parse(stream);
}

web::json::value RequestHandler::convertToCppRest(nlohmann::json const& jsonDoc) {
    utility::stringstream_t stream;
    stream << U(jsonDoc.dump());
    return web::json::value::parse(stream);
}


nlohmann::json RequestHandler::ExtractJsonFromRequest_New(web::http::http_request request) {
    web::json::value jsonBody = web::json::value::null();
    request.extract_json().then([&jsonBody](pplx::task<web::json::value> task)
    {
         try
         {
            jsonBody = task.get();
         }
         catch (web::json::json_exception const & e)
         {
            std::clog << "webserver/error: JSON exception was thrown \"" << e.what() << "\"." << std::endl;
         }
    }).wait();

    return convertFromCppRest(jsonBody);
}


web::json::value RequestHandler::SongsToJsonObject() {
    web::json::value jsonRoot = web::json::value::array();
    for (int i=0; i< m_songs.size(); i++) {
        web::json::value songObject = web::json::value::object();
        songObject["Title"] = web::json::value::string(m_songs[i]->title);
        songObject["Artist"] = web::json::value::string(m_songs[i]->artist);
        songObject["Edition"] = web::json::value::string(m_songs[i]->edition);
        songObject["Language"] = web::json::value::string(m_songs[i]->language);
        songObject["Creator"] = web::json::value::string(m_songs[i]->creator);
        songObject["name"] = web::json::value::string(m_songs[i]->artist + " " + m_songs[i]->title);
        jsonRoot[i] = songObject;
    }

    return jsonRoot;
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

std::shared_ptr<Song> RequestHandler::GetSongFromJSON(web::json::value jsonDoc) {
    m_songs.setFilter("");

    for(int i = 0; i< m_songs.size(); i++) {
        if(m_songs[i]->title == jsonDoc["Title"].as_string() &&
           m_songs[i]->artist == jsonDoc["Artist"].as_string() &&
           m_songs[i]->edition == jsonDoc["Edition"].as_string() &&
           m_songs[i]->language == jsonDoc["Language"].as_string() &&
           m_songs[i]->creator == jsonDoc["Creator"].as_string() ) {
            std::clog << "webserver/info: Found requested song." << std::endl;
            return m_songs[i];
        }
    }

    std::clog << "webserver/info: Couldn't find requested song." << std::endl;
    return std::shared_ptr<Song>();
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
