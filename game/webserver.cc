#include "webserver.hh"
#include <boost/network/protocol/http/server.hpp>

namespace http = boost::network::http;

typedef http::server<WebServer::handler> http_server;

struct WebServer::handler {
	WebServer& m_server;
	void operator() (http_server::request const &request,
	http_server::response &response) {
	//destination describes the file to be loaded, by default it's "/"
		if(request.method == "GET") {
		response = m_server.GETresponse(request);
		} else if(request.method == "POST") {
		response = m_server.POSTresponse(request);
		} else {
		response = http_server::response::stock_reply(
			http_server::response::ok, "other request");
		}
	}

	void log(http_server::string_type const &info) {
		std::cerr << "ERROR: " << info << '\n';
	}
};


void WebServer::StartServer() {
	handler handler_{*this};
	http_server::options options(handler_);
	server_ = new http_server(options.address("0.0.0.0").port("8000"));
	server_->run();
}

WebServer::WebServer(Songs& songs):
m_songs(songs){
	serverthread.reset(new boost::thread(boost::bind(&WebServer::StartServer,boost::ref(*this))));
}

WebServer::~WebServer() {
	server_->stop();
	serverthread->join();
	delete server_;
}

http_server::response WebServer::GETresponse(const http_server::request &request) {
	if(request.destination == "/") { //default
		fs::ifstream f(findFile("index.html"), std::ios::binary);
		f.seekg(0, std::ios::end);
		size_t size = f.tellg();
		f.seekg(0);
		char responseBuffer[size + 2];
		f.read(responseBuffer, size);
		return http_server::response::stock_reply(
		http_server::response::ok, responseBuffer);
		} else if(request.destination == "/api/getDataBase.json") { //get database
		std:: stringstream JSONDB;
		JSONDB << "[\n";
		//no access to upper class yet
		for(int i=0; i<m_songs.size(); i++) {
			JSONDB << "\n{\n\"Title\": \"" << m_songs[i].title << "\"\n\"Artist\": \"";
			JSONDB << m_songs[i].artist << "\"\nEdition\": \"" << m_songs[i].edition << "\"\n\"Language\": \"" << m_songs[i].language;
			JSONDB << "\"\n\"Creator\": \"" << m_songs[i].creator << "\"\n}";
		}
		JSONDB << "\n]";
		return http_server::response::stock_reply(
		http_server::response::ok, JSONDB.str());
	} else if(request.destination == "/api/getCurrentPlaylist.json") { //get playlist
	Game * gm = Game::getSingletonPtr();
	std:: stringstream JSONPlayList;
		JSONPlayList << "[\n";
		for(auto const& song : gm->getCurrentPlayList().getList()) {
			JSONPlayList << "\n{\n\"Title\": \"" << song->title << "\"\n\"Artist\": \"";
			JSONPlayList << song->artist << "\"\n\"Edition\": \"" << song->edition << "\"\n\"Language\": \"" << song->language;
			JSONPlayList << "\"\n\"Creator\": \"" << song->creator << "\"\n}";
		}
		JSONPlayList << "\n]";
		return http_server::response::stock_reply(
		http_server::response::ok,JSONPlayList.str());

	} else {
		//other text files
		try {
		std::string destination = request.destination;
		destination.erase(0,1);
		fs::ifstream f(findFile(destination), std::ios::binary);
		f.seekg(0, std::ios::end);
		size_t size = f.tellg();
		f.seekg(0);
		char responseBuffer[size + 2];
		f.read(responseBuffer, size);
		return http_server::response::stock_reply(
		http_server::response::ok, responseBuffer);
		}
		catch(std::exception e) {
		return http_server::response::stock_reply(
		http_server::response::ok, "not a text file");
		}
	}
}

http_server::response WebServer::POSTresponse(const http_server::request &request) {
return http_server::response::stock_reply(
		http_server::response::ok, "not yet implemented");
}
