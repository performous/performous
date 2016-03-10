#include "webserver.hh"
#ifdef USE_CPPNETLIB
#include <boost/network/protocol/http/server.hpp>

namespace http = boost::network::http;

typedef http::server<WebServer::handler> http_server;

struct WebServer::handler {
	WebServer& m_server;

	void operator() (http_server::request const &request, http_server::response &response) {
		// Destination describes the file to be loaded, by default it's "/"
		if (request.method == "GET") {
			response = m_server.GETresponse(request);
		} else if (request.method == "POST") {
			response = m_server.POSTresponse(request);
		} else {
			response = http_server::response::stock_reply(
			http_server::response::ok, "other request");
		}
	}

	void log(http_server::string_type const &info) {
		std::clog << "webserver/error: " << info << std::endl;
	}
};


void WebServer::StartServer() {
	handler handler_{*this};
	http_server::options options(handler_);
	if(config["game/webserver_access"].i() == 1) {
		std::clog << "webserver/notice: Starting local server." << std::endl;
		m_server.reset(new http_server(options.address("127.0.0.1").port(std::to_string(config["game/webserver_port"].i()))));
	} else {
		m_server.reset(new http_server(options.address("0.0.0.0").port(std::to_string(config["game/webserver_port"].i()))));
		std::clog << "webserver/notice: Starting public server." << std::endl;
	}
	m_server->run();
}

WebServer::WebServer(Songs& songs)
: m_songs(songs)
{
	if(config["game/webserver_access"].i() == 0) {
		std::clog << "webserver/notice: Not starting webserver." << std::endl;
	} else {
		m_serverThread.reset(new boost::thread(boost::bind(&WebServer::StartServer,boost::ref(*this))));
	}
}

WebServer::~WebServer() {
	m_server->stop();
	m_serverThread->join();
}

http_server::response WebServer::GETresponse(const http_server::request &request) {
	if (request.destination == "/") { //default
		BinaryBuffer buf = readFile(findFile("index.html"));
		return http_server::response::stock_reply(http_server::response::ok, std::string(buf.begin(), buf.end()));
	} else if (request.destination == "/api/getDataBase.json") { //get database
		m_songs.setFilter("");
		Json::Value jsonRoot = Json::arrayValue;
		for (int i=0; i<m_songs.size(); i++) {
			Json::Value SongObject = Json::objectValue;
			SongObject["Title"] = m_songs[i]->title;
			SongObject["Artist"] = m_songs[i]->artist;
			SongObject["Edition"] = m_songs[i]->edition;
			SongObject["Language"] = m_songs[i]->language;
			SongObject["Creator"] = m_songs[i]->creator;
			jsonRoot.append(SongObject);
		}
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
				jsonRoot.append(SongObject);
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
			destination.erase(0,1);//remove the first /
			BinaryBuffer buf = readFile(findFile(destination));
			return http_server::response::stock_reply(http_server::response::ok, std::string(buf.begin(), buf.end()));
		}
		catch(std::exception e) {
			return http_server::response::stock_reply(http_server::response::ok, "not a text file");
		}
	}
}

http_server::response WebServer::POSTresponse(const http_server::request &request) {
	Game * gm = Game::getSingletonPtr();
	if (request.destination == "/api/add") {
			m_songs.setFilter("");
		boost::shared_ptr<Song> pointer = GetSongFromJSON(request.body);
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
			int songToDelete = boost::lexical_cast<int>(request.body);
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
	} else if (request.destination == "/api/moveup") {
		try {
			int songToMove = boost::lexical_cast<int>(request.body);
			if(songToMove > 0) {
				gm->getCurrentPlayList().swap(songToMove,songToMove -1);
			}
			ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
			m_pp->triggerSongListUpdate(); //trigger live-update
			return http_server::response::stock_reply(http_server::response::ok, "ok");
		} catch(std::exception e) {
			return http_server::response::stock_reply(http_server::response::ok, "failure");
		}
	} else if (request.destination == "/api/movedown") {
		try {
			unsigned int songToMove = boost::lexical_cast<int>(request.body);
			if(songToMove < gm->getCurrentPlayList().getList().size() -1) {
				gm->getCurrentPlayList().swap(songToMove,songToMove + 1);
			}
			ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
			m_pp->triggerSongListUpdate(); //trigger live-update
			return http_server::response::stock_reply(http_server::response::ok, "ok");
		} catch(std::exception e) {
			return http_server::response::stock_reply(http_server::response::ok, "failure");
		}
	} else {
		return http_server::response::stock_reply(http_server::response::ok, "not yet implemented");
	}
}

boost::shared_ptr<Song> WebServer::GetSongFromJSON(std::string JsonDoc) {
	Json::Value root;   // will contains the root value after parsing.
	Json::Reader reader;
	bool parsingSuccessful = reader.parse(JsonDoc,root);
	if(!parsingSuccessful) {
		std::clog << "webserver/error: cannot parse Json Document" <<std::endl;
		return boost::shared_ptr<Song>();
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

	return boost::shared_ptr<Song>();
}
#else
WebServer::WebServer(Songs& songs)
	: m_songs(songs) {}
WebServer::~WebServer(){}
#endif
