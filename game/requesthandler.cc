#include "requesthandler.hh"
#include "unicode.hh"

#ifdef USE_WEBSERVER
RequestHandler::RequestHandler(Songs& songs):m_songs(songs)
{
}

RequestHandler::RequestHandler(std::string url, Songs& songs): m_listener(utility::conversions::to_string_t(url)), m_songs(songs)
{
    m_listener.support(web::http::methods::GET, std::bind(&RequestHandler::Get, this, std::placeholders::_1));
    m_listener.support(web::http::methods::PUT, std::bind(&RequestHandler::Put, this, std::placeholders::_1));
    m_listener.support(web::http::methods::POST, std::bind(&RequestHandler::Post, this, std::placeholders::_1));
    m_listener.support(web::http::methods::DEL, std::bind(&RequestHandler::Delete, this, std::placeholders::_1));

}
RequestHandler::~RequestHandler()
{
}

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

void RequestHandler::HandleFile(web::http::http_request request, std::string filePath) {
    auto path = filePath != "" ? filePath : utility::conversions::to_utf8string(request.relative_uri().path());
    auto fileName = path.substr(path.find_last_of("/\\") + 1);
        
    std::string fileToSend = findFile(fileName).string();

    concurrency::streams::fstream::open_istream(utility::conversions::to_string_t(fileToSend), std::ios::in).then([=](concurrency::streams::istream is) {
        std::string content_type = "";
        if(path.find(".html") != std::string::npos) {
            content_type = "text/html";
        } else if(path.find(".js") != std::string::npos) {
            content_type = "text/javascript";
        } else if (path.find(".css") != std::string::npos) {
            content_type = "text/css";
        } else if (path.find(".png") != std::string::npos) {
            content_type = "image/png";
        } else if (path.find(".gif") != std::string::npos) {
            content_type = "image/gif";
        } else if (path.find(".ico") != std::string::npos) {
            content_type = "image/x-icon";
        }

        request.reply(web::http::status_codes::OK, is, utility::conversions::to_string_t(content_type)).then([](pplx::task<void> t) {
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
            request.reply(web::http::status_codes::InternalError, "INTERNAL ERROR");
        }
    });
}

void RequestHandler::Get(web::http::http_request request)
{
    std::string content_type = "text/html";
    auto path = utility::conversions::to_utf8string(request.relative_uri().path());
    auto query = utility::conversions::to_utf8string(request.relative_uri().query());

    if(query != "") {
        path += "?" + query;
    }
    std::clog << "requesthandler/debug: path is: " << path << std::endl;
    if (path == "/") {
        HandleFile(request, findFile("index.html").string());
    } else if (path == "/api/getDataBase.json") { //get database
        m_songs.setFilter("");
        if(query == "sort=artist&order=ascending") {
            m_songs.sortSpecificChange(2);    
        } else if(query == "sort=artist&order=descending") {
            m_songs.sortSpecificChange(2, true);
        } else if(query == "sort=title&order=ascending") {
            m_songs.sortSpecificChange(1);    
        } else if(query == "sort=title&order=descending") {
            m_songs.sortSpecificChange(1, true);
        } else if(query == "sort=language&order=ascending") {
            m_songs.sortSpecificChange(6);    
        } else if(query == "sort=language&order=descending") {
            m_songs.sortSpecificChange(6, true);
        } else if(query == "sort=edition&order=ascending") {
            m_songs.sortSpecificChange(3);    
        } else if(query == "sort=edition&order=descending") {
            m_songs.sortSpecificChange(3, true);
        }
        web::json::value jsonRoot = SongsToJsonObject();
        request.reply(web::http::status_codes::OK, jsonRoot);
        return;
    }  else if(path == "/api/language") {
        auto localeMap = GenerateLocaleDict();
        web::json::value jsonRoot = web::json::value::object();
            for (auto const &kv : localeMap) {
                std::string key = kv.first;
                //Hack to get an easy key value pair within the json object.
                if(key == "Web interface by Niek Nooijens and Arjan Speiard, for full credits regarding Performous see /docs/Authors.txt"){
                    key = "Credits";
                }
                std::replace(key.begin(), key.end(), ' ','_');
                key = UnicodeUtil::toLower(key);;
                jsonRoot[utility::conversions::to_string_t(key)] = web::json::value(utility::conversions::to_string_t(kv.second));
            }
        request.reply(web::http::status_codes::OK, jsonRoot);
        return;
    } else if(path == "/api/getCurrentPlaylist.json") {
        Game* gm = Game::getSingletonPtr();
        web::json::value jsonRoot = web::json::value::array();
        auto i = 0;
        for (auto const& song : gm->getCurrentPlayList().getList()) {
            web::json::value songObject = web::json::value::object();
            songObject[utility::conversions::to_string_t("Title")] = web::json::value::string(utility::conversions::to_string_t(song->title));
            songObject[utility::conversions::to_string_t("Artist")] = web::json::value::string(utility::conversions::to_string_t(song->artist));
            songObject[utility::conversions::to_string_t("Edition")] = web::json::value::string(utility::conversions::to_string_t(song->edition));
            songObject[utility::conversions::to_string_t("Language")] = web::json::value::string(utility::conversions::to_string_t(song->language));
            songObject[utility::conversions::to_string_t("Creator")] = web::json::value::string(utility::conversions::to_string_t(song->creator));
            songObject[utility::conversions::to_string_t("Duration")] = web::json::value(song->getDurationSeconds());
            jsonRoot[i] = songObject;
            i++;
        }

        request.reply(web::http::status_codes::OK, jsonRoot);
        return;
    } else if(path == "/api/getplaylistTimeout") {
        request.reply(web::http::status_codes::OK, config["game/playlist_screen_timeout"].i());
        return;
    } else {
        HandleFile(request);
    }
};

void RequestHandler::Post(web::http::http_request request)
{
    Game* gm = Game::getSingletonPtr();

    auto path = utility::conversions::to_utf8string(request.relative_uri().path());
    auto query = utility::conversions::to_utf8string(request.relative_uri().query());
    if(query != "") {
        path += "?" + query;
    }
    std::clog << "requesthandler/debug: path is: " << path << std::endl;

    web::json::value jsonPostBody = ExtractJsonFromRequest(request);

    if(jsonPostBody == web::json::value::null()) {
        request.reply(web::http::status_codes::BadRequest, "Post body is malformed. Please make a valid request.");
        return;
    }

    if (path == "/api/add") {
        m_songs.setFilter("");
        std::shared_ptr<Song> songPointer = GetSongFromJSON(jsonPostBody);
        if(!songPointer) {
            request.reply(web::http::status_codes::NotFound, "Song \"" + utility::conversions::to_utf8string(jsonPostBody[utility::conversions::to_string_t("Artist")].as_string()) + " - " + utility::conversions::to_utf8string(jsonPostBody[utility::conversions::to_string_t("Title")].as_string()) + "\" was not found.");
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
            auto songIdToDelete = jsonPostBody[utility::conversions::to_string_t("songId")].as_integer();
            if(songIdToDelete >= 0) {
                gm->getCurrentPlayList().removeSong(songIdToDelete);
                ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
                m_pp->triggerSongListUpdate();

                request.reply(web::http::status_codes::OK, "success");
                return;
            } else {
                request.reply(web::http::status_codes::BadRequest, "Can't remove songs from the playlist with a negative id \"" + std::to_string(songIdToDelete) +"\". Please make a valid request.");
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
            auto songIdToMove = jsonPostBody[utility::conversions::to_string_t("songId")].as_integer();
            auto positionToMoveTo = jsonPostBody[utility::conversions::to_string_t("position")].as_integer();
            int sizeOfPlaylist = gm->getCurrentPlayList().getList().size();
            if(songIdToMove < 0) {
                request.reply(web::http::status_codes::BadRequest, "Can't move songs with a negative id \"" + std::to_string(songIdToMove) + "\". Please make a valid request.");
                return;
            }
            if(positionToMoveTo < 0) {
                request.reply(web::http::status_codes::BadRequest, "Can't move songs to a negative position \"" + std::to_string(positionToMoveTo) + "\". Please make a valid request.");
                return;
            }
            if(songIdToMove > sizeOfPlaylist - 1) {
                request.reply(web::http::status_codes::BadRequest, "Not gonna move the unknown song you've provided \"" + std::to_string(songIdToMove + 1) + "\". Please make a valid request.");
                return;
            }
            if(positionToMoveTo <= sizeOfPlaylist - 1) {
                gm->getCurrentPlayList().setPosition(songIdToMove,positionToMoveTo);
                ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
                m_pp->triggerSongListUpdate();
                request.reply(web::http::status_codes::OK, "success");
                return;
            } else  {
                request.reply(web::http::status_codes::BadRequest, "Not gonna move the song to \""+ std::to_string(positionToMoveTo + 1) + "\" since the list ain't that long. Please make a valid request.");
                return;
            }
        } catch(web::json::json_exception const & e) {
            std::string str = std::string("JSON Exception: ") + e.what();
            request.reply(web::http::status_codes::BadRequest, str);
            return;
        }     
    } else if(path == "/api/search") {
        auto query = utility::conversions::to_utf8string(jsonPostBody[utility::conversions::to_string_t("query")].as_string());
        m_songs.setFilter(query);
        web::json::value jsonRoot = web::json::value::array();
        for(int i = 0; i < m_songs.size(); i++) {
            web::json::value songObject = web::json::value::object();
            songObject[utility::conversions::to_string_t("Title")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->title));
            songObject[utility::conversions::to_string_t("Artist")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->artist));
            songObject[utility::conversions::to_string_t("Edition")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->edition));
            songObject[utility::conversions::to_string_t("Language")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->language));
            songObject[utility::conversions::to_string_t("Creator")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->creator));
            jsonRoot[i] = songObject;
        }
        request.reply(web::http::status_codes::OK, jsonRoot);
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


web::json::value RequestHandler::SongsToJsonObject() {
    web::json::value jsonRoot = web::json::value::array();
    for (int i=0; i< m_songs.size(); i++) {
        web::json::value songObject = web::json::value::object();
        songObject[utility::conversions::to_string_t("Title")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->title));
        songObject[utility::conversions::to_string_t("Artist")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->artist));
        songObject[utility::conversions::to_string_t("Edition")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->edition));
        songObject[utility::conversions::to_string_t("Language")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->language));
        songObject[utility::conversions::to_string_t("Creator")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->creator));
        songObject[utility::conversions::to_string_t("name")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->artist + " " + m_songs[i]->title));
        jsonRoot[i] = songObject;
    }

    return jsonRoot;
}

std::shared_ptr<Song> RequestHandler::GetSongFromJSON(web::json::value jsonDoc) {
    m_songs.setFilter("");

    for(int i = 0; i< m_songs.size(); i++) {
        if(m_songs[i]->title == utility::conversions::to_utf8string(jsonDoc[utility::conversions::to_string_t("Title")].as_string()) &&
           m_songs[i]->artist == utility::conversions::to_utf8string(jsonDoc[utility::conversions::to_string_t("Artist")].as_string()) &&
           m_songs[i]->edition == utility::conversions::to_utf8string(jsonDoc[utility::conversions::to_string_t("Edition")].as_string()) &&
           m_songs[i]->language == utility::conversions::to_utf8string(jsonDoc[utility::conversions::to_string_t("Language")].as_string()) &&
           m_songs[i]->creator == utility::conversions::to_utf8string(jsonDoc[utility::conversions::to_string_t("Creator")].as_string()) ) {
            std::clog << "webserver/info: Found requested song." << std::endl;
            return m_songs[i];
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
