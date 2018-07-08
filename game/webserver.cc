#include "webserver.hh"
#ifdef USE_WEBSERVER
#include "chrono.hh"
#include <boost/network/protocol/http/server.hpp>
#include <thread>

namespace http = boost::network::http;

typedef http::server<WebServer::handler> http_server;

struct WebServer::handler {
	WebServer& m_server;

	void operator() (http_server::request const &request, http_server::response &response) {
		// Destination describes the file to be loaded, by default it's "/"
		std::string content_type = "text/html";
		if (request.method == "GET") {
			response = m_server.GETresponse(request, content_type);
		} else if (request.method == "POST") {
			response = m_server.POSTresponse(request);
		} else {
			response = http_server::response::stock_reply(
			http_server::response::ok, "other request");
		}
		for(unsigned int i=0; i< response.headers.size(); i++) {
			if(response.headers.at(i).name == "Content-Type") {
				response.headers.at(i).value = content_type;
			}
		}
	}

	void log(http_server::string_type const &info) {
		std::clog << "webserver/error: " << info << std::endl;
	}
};


void WebServer::StartServer(int tried, bool fallbackPortInUse = false) {
	if(tried > 2) {
		if(fallbackPortInUse == false) {
			std::clog << "webserver/error: Couldn't start webserver tried 3 times. Trying fallbackport.." << std::endl;
			StartServer(0, true);
			return;
		}

		std::clog << "webserver/error: Couldn't start webserver tried 3 times using normal port and 3 times using fallback port. Stopping webserver." << std::endl; 
		if(m_server) {
			m_server->stop();
		}
		return;
	}

	handler handler_{*this};
	http_server::options options(handler_);
	std::string portToUse = fallbackPortInUse ? std::to_string(config["game/webserver_fallback_port"].i()) : std::to_string(config["game/webserver_port"].i());
	if(config["game/webserver_access"].i() == 1) {
		std::clog << "webserver/notice: Starting local server..." << std::endl;
		m_server.reset(new http_server(options.address("127.0.0.1").port(portToUse)));
	} else {
		m_server.reset(new http_server(options.address("0.0.0.0").port(portToUse)));
		std::clog << "webserver/notice: Starting public server..." << std::endl;
	}
	try {
		m_server->run();
	}
	catch (std::exception& e)
	{
		tried = tried + 1;
		std::clog << "webserver/error: " << e.what() << " Trying again... (tried " << tried << " times)." << std::endl;
		std::this_thread::sleep_for(20s);
		StartServer(tried, fallbackPortInUse);
	}
}

WebServer::WebServer(Songs& songs)
: m_songs(songs)
{
	if(config["game/webserver_access"].i() == 0) {
		std::clog << "webserver/notice: Not starting webserver." << std::endl;
	} else {
		m_serverThread = std::make_unique<std::thread>([this] { StartServer(0, false); });
	}
}

WebServer::~WebServer() {
	if ( m_server ) {
		m_server->stop();
		m_serverThread->join();
	}
}

Json::Value WebServer::SongsToJsonObject(){
	Json::Value jsonRoot = Json::arrayValue;
	for (int i=0; i<m_songs.size(); i++) {
		Json::Value SongObject = Json::objectValue;
		SongObject["Title"] = m_songs[i]->title;
		SongObject["Artist"] = m_songs[i]->artist;
		SongObject["Edition"] = m_songs[i]->edition;
		SongObject["Language"] = m_songs[i]->language;
		SongObject["Creator"] = m_songs[i]->creator;
		//SongObject["Duration"] = m_songs[i]->getDurationSeconds();
		SongObject["name"] = m_songs[i]->artist + " - " + m_songs[i]->title;
		jsonRoot.append(SongObject);
	}

	return jsonRoot;
}

http_server::response WebServer::GETresponse(const http_server::request &request, std::string& content_type) {
	content_type = "text/html";
	if (request.destination == "/") { //default
		BinaryBuffer buf = readFile(findFile("index.html"));
		return http_server::response::stock_reply(http_server::response::ok, std::string(buf.begin(), buf.end()));
	} else if (request.destination == "/api/getDataBase.json") { //get database
		m_songs.setFilter("");
		Json::Value jsonRoot = SongsToJsonObject();
		return http_server::response::stock_reply(http_server::response::ok, jsonRoot.toStyledString());
	} else if (request.destination == "/api/getDataBase.json?sort=artist&order=ascending") { //get database
		m_songs.setFilter("");
		m_songs.sortSpecificChange(2);
		Json::Value jsonRoot = SongsToJsonObject();
		return http_server::response::stock_reply(http_server::response::ok, jsonRoot.toStyledString());
	} else if (request.destination == "/api/getDataBase.json?sort=artist&order=descending") { //get database
		m_songs.setFilter("");
		m_songs.sortSpecificChange(2, true);
		Json::Value jsonRoot = SongsToJsonObject();
		return http_server::response::stock_reply(http_server::response::ok, jsonRoot.toStyledString());
	} else if (request.destination == "/api/getDataBase.json?sort=title&order=ascending") { //get database
		m_songs.setFilter("");
		m_songs.sortSpecificChange(1);
		Json::Value jsonRoot = SongsToJsonObject();
		return http_server::response::stock_reply(http_server::response::ok, jsonRoot.toStyledString());
	} else if (request.destination == "/api/getDataBase.json?sort=title&order=descending") { //get database
		m_songs.setFilter("");
		m_songs.sortSpecificChange(1, true);
		Json::Value jsonRoot = SongsToJsonObject();
		return http_server::response::stock_reply(http_server::response::ok, jsonRoot.toStyledString());
	} else if (request.destination == "/api/getDataBase.json?sort=language&order=ascending") { //get database
		m_songs.setFilter("");
		m_songs.sortSpecificChange(6);
		Json::Value jsonRoot = SongsToJsonObject();
		return http_server::response::stock_reply(http_server::response::ok, jsonRoot.toStyledString());
	} else if (request.destination == "/api/getDataBase.json?sort=language&order=descending") { //get database
		m_songs.setFilter("");
		m_songs.sortSpecificChange(6, true);
		Json::Value jsonRoot = SongsToJsonObject();
		return http_server::response::stock_reply(http_server::response::ok, jsonRoot.toStyledString());
	}else if (request.destination == "/api/getDataBase.json?sort=edition&order=ascending") { //get database
		m_songs.setFilter("");
		m_songs.sortSpecificChange(3);
		Json::Value jsonRoot = SongsToJsonObject();
		return http_server::response::stock_reply(http_server::response::ok, jsonRoot.toStyledString());
	} else if (request.destination == "/api/getDataBase.json?sort=edition&order=descending") { //get database
		m_songs.setFilter("");
		m_songs.sortSpecificChange(3, true);
		Json::Value jsonRoot = SongsToJsonObject();
		return http_server::response::stock_reply(http_server::response::ok, jsonRoot.toStyledString());
	} else if (request.destination == "/api/getCurrentPlaylist.json") { //get playlist
		Game* gm = Game::getSingletonPtr();
		Json::Value jsonRoot = Json::arrayValue;
		for (auto const& song : gm->getCurrentPlayList().getList()) {
				Json::Value SongObject = Json::objectValue;
				SongObject["Title"] = song->title;
				SongObject["Artist"] = song->artist;
				SongObject["Edition"] = song->edition;
				SongObject["Language"] = song->language;
				SongObject["Creator"] = song->creator;
				SongObject["Duration"] = song->getDurationSeconds();
				jsonRoot.append(SongObject);
		}
		return http_server::response::stock_reply(http_server::response::ok, jsonRoot.toStyledString());
	} else if(request.destination == "/api/getplaylistTimeout") {
		return http_server::response::stock_reply(http_server::response::ok, std::to_string(config["game/playlist_screen_timeout"].i()));
	} else if(request.destination.find("/api/language") == 0) {
		auto localeMap = GenerateLocaleDict();
		
		Json::Value jsonRoot = Json::objectValue;
		for (auto const &kv : localeMap) {
			std::string key = kv.first;
			//Hack to get an easy key value pair within the json object.
			if(key == "Web interface by Niek Nooijens and Arjan Speiard, for full credits regarding Performous see /docs/Authors.txt"){
				key = "Credits";
			}
			std::replace(key.begin(), key.end(), ' ','_');
			boost::to_lower(key);
			jsonRoot[key] = kv.second;
		}
		return http_server::response::stock_reply(http_server::response::ok, jsonRoot.toStyledString());
	} else {
		//other text files
		try {
			std::string destination = request.destination;
			///this is to make sure the tree is only accessible downwards
			while(destination[1] == '.' && destination[2] == '.' && destination[3] == '/') {
				destination.erase(1,4);
			}
			destination.erase(0,destination.find_last_of('/') + 1);//we're only intrested in the filename
			fs::path fileToSend = findFile(destination);
			BinaryBuffer buf = readFile(fileToSend);
			http_server::response m_response = http_server::response::stock_reply(http_server::response::ok, std::string(buf.begin(), buf.end()));
			if(destination.find(".js") != std::string::npos) { //javascript!!
				content_type = "text/javascript";
			} else if (destination.find(".css") != std::string::npos) { //stylesheet
				content_type = "text/css";
			} else if (destination.find(".png") != std::string::npos) { //other icons
				content_type = "image/png";
			} else if (destination.find(".gif") != std::string::npos) { //gif for loading animation
				content_type = "image/gif";
			} else if (destination.find(".ico") != std::string::npos) { //ico for page icon
				 content_type = "image/x-icon";
			}
			return m_response;

		}
		catch(std::exception e) {
			return http_server::response::stock_reply(http_server::response::not_found, "notfound");
		}
	}
}

http_server::response WebServer::POSTresponse(const http_server::request &request) {
	Game * gm = Game::getSingletonPtr();
	if (request.destination == "/api/add") {
			m_songs.setFilter("");
		std::shared_ptr<Song> pointer = GetSongFromJSON(request.body);
		if (!pointer) {
			return http_server::response::stock_reply(
			http_server::response::ok, "failure");
		} else {
			std::clog << "webserver/debug: Add " << pointer->title << std::endl;
			gm->getCurrentPlayList().addSong(pointer);
			ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
			m_pp->triggerSongListUpdate(); //this way the screen_playlist does a live-update just like the screen_songs
			return http_server::response::stock_reply(http_server::response::ok, "success");
		}
	} else if(request.destination == "/api/remove") {
		try { // this is for those idiots that send text instead of numbers.
			int songToDelete = std::stoi(request.body);
			gm->getCurrentPlayList().removeSong(songToDelete);
			ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
			m_pp->triggerSongListUpdate(); //this way the screen_playlist does a live-update just like the screen_songs
			return http_server::response::stock_reply(http_server::response::ok, "success");
		} catch(std::exception e) {
			return http_server::response::stock_reply(http_server::response::ok, "failure");
		}
	} else if (request.destination == "/api/autocomplete" || request.destination == "/api/search") {
			m_songs.setFilter(request.body); //set filter and get the results
			Json::Value jsonRoot = Json::arrayValue;
			for (int i=0; i< m_songs.size(); i++) {
				if(i > 10 && request.destination == "/api/autocomplete") break; //only break at autocomplete
				Json::Value SongObject = Json::objectValue;
				SongObject["Title"] = m_songs[i]->title;
				SongObject["Artist"] = m_songs[i]->artist;
				SongObject["Edition"] = m_songs[i]->edition;
				SongObject["Language"] = m_songs[i]->language;
				SongObject["Creator"] = m_songs[i]->creator;
				jsonRoot.append(SongObject);
			}
			return http_server::response::stock_reply(http_server::response::ok, jsonRoot.toStyledString());
	} else if (request.destination == "/api/setposition") {
		try {
				Json::Value root;
				Json::Reader reader;
				bool parsingSuccessful = reader.parse(request.body, root);
				if(!parsingSuccessful) {
					std::clog << "webserver/error: cannot parse Json Document" <<std::endl;
					return http_server::response::stock_reply(http_server::response::ok, "No valid JSON.");
				}
				unsigned int songToMove = root["songId"].asUInt();
				unsigned int positionToMoveTo = root["position"].asUInt();

				if(gm->getCurrentPlayList().getList().size() == 0) {
					return http_server::response::stock_reply(http_server::response::ok, "Playlist is empty, can't move the song you've provided: " + std::to_string(songToMove + 1));
				}

				if(songToMove > gm->getCurrentPlayList().getList().size() -1) {
					return http_server::response::stock_reply(http_server::response::ok, "Not gonna move the unknown song you've provided: " + std::to_string(songToMove + 1));
				}

				if(positionToMoveTo <= gm->getCurrentPlayList().getList().size() -1) {
					gm->getCurrentPlayList().setPosition(songToMove,positionToMoveTo);
					ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
					m_pp->triggerSongListUpdate();
				} else {
					return http_server::response::stock_reply(http_server::response::ok, "Not gonna move the song to "+ std::to_string(positionToMoveTo + 1) + " since the list ain't that long.");
				}
			return http_server::response::stock_reply(http_server::response::ok, "Succesfuly moved the song from " + std::to_string(songToMove + 1) + " to " + std::to_string(positionToMoveTo + 1));
		} catch(std::exception e) {
			return http_server::response::stock_reply(http_server::response::ok, "failure");
		}
	} else {
		return http_server::response::stock_reply(http_server::response::ok, "not yet implemented");
	}
}

std::map<std::string, std::string> WebServer::GenerateLocaleDict() {
	std::vector<std::string> translationKeys = GetTranslationKeys();
	std::map<std::string, std::string> localeMap;
	for (auto const &translationKey : translationKeys) {
		localeMap[translationKey] = _(translationKey);
	}
	return localeMap;
}

std::vector<std::string> WebServer::GetTranslationKeys() {
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

std::shared_ptr<Song> WebServer::GetSongFromJSON(std::string JsonDoc) {
	Json::Value root;   // will contains the root value after parsing.
	Json::Reader reader;
	bool parsingSuccessful = reader.parse(JsonDoc,root);
	if(!parsingSuccessful) {
		std::clog << "webserver/error: cannot parse Json Document" <<std::endl;
		return std::shared_ptr<Song>();
	}
	m_songs.setFilter("");
	for(int i = 0; i< m_songs.size(); i++) {
		if(m_songs[i]->title == root["Title"].asString() &&
				m_songs[i]->artist == root["Artist"].asString() &&
				m_songs[i]->edition == root["Edition"].asString() &&
				m_songs[i]->language == root["Language"].asString() &&
				m_songs[i]->creator == root["Creator"].asString()) {
			return m_songs[i];
		}
	}

	return std::shared_ptr<Song>();
}
#endif
