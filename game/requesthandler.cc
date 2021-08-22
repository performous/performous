#include "requesthandler.hh"
#include "unicode.hh"
#include "game.hh"

#ifdef USE_WEBSERVER
RequestHandler::RequestHandler(Game &game, Songs& songs): m_game(game), m_songs(songs)
{
}
RequestHandler::RequestHandler(Game &game, std::string url, Songs& songs): m_game(game), m_listener(url),m_songs(songs)
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
    auto path = filePath != "" ? filePath : request.relative_uri().path();
    auto fileName = path.substr(path.find_last_of("/\\") + 1);

    std::string fileToSend = findFile(fileName).string();

    concurrency::streams::fstream::open_istream(U(fileToSend), std::ios::in).then([=](concurrency::streams::istream is) {
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
                jsonRoot[key] = web::json::value(kv.second);
            }
        request.reply(web::http::status_codes::OK, jsonRoot);
        return;
    } else if(path == "/api/getCurrentPlaylist.json") {
        web::json::value jsonRoot = web::json::value::array();
        auto i = 0;
        for (auto const& song : m_game.getCurrentPlayList().getList()) {
            web::json::value songObject = web::json::value::object();
            songObject["Title"] = web::json::value::string(song->title);
            songObject["Artist"] = web::json::value::string(song->artist);
            songObject["Edition"] = web::json::value::string(song->edition);
            songObject["Language"] = web::json::value::string(song->language);
            songObject["Creator"] = web::json::value::string(song->creator);
            songObject["Duration"] = web::json::value(song->getDurationSeconds());
            jsonRoot[i] = songObject;
            i++;
        }

        request.reply(web::http::status_codes::OK, jsonRoot);
        return;
    } else if(path == "/api/getplaylistTimeout") {
        request.reply(web::http::status_codes::OK, U(config["game/playlist_screen_timeout"].i()));
        return;
    } else {
        HandleFile(request);
    }
}

void RequestHandler::Post(web::http::http_request request)
{
    auto uri = request.relative_uri().path();
    if(request.relative_uri().query() != "") {
        uri += "?" + request.relative_uri().query();
    }
    std::clog << "requesthandler/debug: path is: " << uri << std::endl;

    auto path = request.relative_uri().path();

    web::json::value jsonPostBody = ExtractJsonFromRequest(request);

    if(jsonPostBody == web::json::value::null()) {
        request.reply(web::http::status_codes::BadRequest, "Post body is malformed. Please make a valid request.");
        return;
    }

    if (path == "/api/add") {
        m_songs.setFilter("");
        std::shared_ptr<Song> songPointer = GetSongFromJSON(jsonPostBody);
        if(!songPointer) {
            request.reply(web::http::status_codes::NotFound, "Song \"" + jsonPostBody["Artist"].as_string() + " - " + jsonPostBody["Title"].as_string() + "\" was not found.");
            return;
        } else {
            std::clog << "requesthandler/debug: Adding " << songPointer->artist << " - " << songPointer->title << " to the playlist " << std::endl;
            m_game.getCurrentPlayList().addSong(songPointer);
            ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(m_game.getScreen("Playlist"));
            m_pp->triggerSongListUpdate();

            request.reply(web::http::status_codes::OK, "success");
            return;
        }
    } else if(path == "/api/remove") {
        if(m_game.getCurrentPlayList().isEmpty()) {
            request.reply(web::http::status_codes::BadRequest, "Playlist is empty.");
            return;
        }
        try {
            auto songIdToDelete = jsonPostBody["songId"].as_integer();
            if(songIdToDelete >= 0) {
                m_game.getCurrentPlayList().removeSong(songIdToDelete);
                ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(m_game.getScreen("Playlist"));
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
        if(m_game.getCurrentPlayList().isEmpty()) {
            request.reply(web::http::status_codes::BadRequest, "Playlist is empty.");
            return;
        }
        try {
            auto songIdToMove = jsonPostBody["songId"].as_integer();
            auto positionToMoveTo = jsonPostBody["position"].as_integer();
            int sizeOfPlaylist = m_game.getCurrentPlayList().getList().size();
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
                m_game.getCurrentPlayList().setPosition(songIdToMove,positionToMoveTo);
                ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(m_game.getScreen("Playlist"));
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
        auto query = jsonPostBody["query"].as_string();
        m_songs.setFilter(query);
        web::json::value jsonRoot = web::json::value::array();
        for(int i = 0; i < m_songs.size(); i++) {
            web::json::value songObject = web::json::value::object();
            songObject["Title"] = web::json::value::string(m_songs[i]->title);
            songObject["Artist"] = web::json::value::string(m_songs[i]->artist);
            songObject["Edition"] = web::json::value::string(m_songs[i]->edition);
            songObject["Language"] = web::json::value::string(m_songs[i]->language);
            songObject["Creator"] = web::json::value::string(m_songs[i]->creator);
            jsonRoot[i] = songObject;
        }
        request.reply(web::http::status_codes::OK, jsonRoot);
        return;
    } else {
        request.reply(web::http::status_codes::NotFound, "The path \""+ path +"\" was not found.");
        return;
    }
}

void RequestHandler::Delete(web::http::http_request request)
{
    request.reply(web::http::status_codes::OK);
}

void RequestHandler::Put(web::http::http_request request)
{
    request.reply(web::http::status_codes::OK);
}

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
