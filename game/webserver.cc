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
		for(int i=0; i<m_songs.size(); i++) {
			JSONDB << "\n{\n\"Title\": \"" << m_songs[i].title << "\",\n\"Artist\": \"";
			JSONDB << m_songs[i].artist << "\",\n\"Edition\": \"" << m_songs[i].edition << "\",\n\"Language\": \"" << m_songs[i].language;
			JSONDB << "\",\n\"Creator\": \"" << m_songs[i].creator << "\"\n},";
		}
		std::string output = JSONDB.str(); //remove the last comma
		output.pop_back(); //remove the last comma
		output += "\n]";
		return http_server::response::stock_reply(
		http_server::response::ok, output);
	} else if(request.destination == "/api/getCurrentPlaylist.json") { //get playlist
	Game * gm = Game::getSingletonPtr();
	std:: stringstream JSONPlayList;
		JSONPlayList << "[\n";
		for(auto const& song : gm->getCurrentPlayList().getList()) {
			JSONPlayList << "\n{\n\"Title\": \"" << song->title << "\",\n\"Artist\": \"";
			JSONPlayList << song->artist << "\",\n\"Edition\": \"" << song->edition << "\",\n\"Language\": \"" << song->language;
			JSONPlayList << "\",\n\"Creator\": \"" << song->creator << "\"\n},";
		}
		std::string output = JSONPlayList.str();
		output.pop_back(); //remove the last comma
		output += "\n]";
		return http_server::response::stock_reply(
		http_server::response::ok,output);

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
	if(request.destination == "/api/add") {
		Game * gm = Game::getSingletonPtr();
		boost::shared_ptr<Song> pointer = GetSongFromJSON(request.body);
		if (!pointer) {
			return http_server::response::stock_reply(
			http_server::response::ok, "failure");
		} else {
		std::cout << pointer->title << std::endl;
		gm->getCurrentPlayList().addSong(pointer);
		return http_server::response::stock_reply(
			http_server::response::ok, "success");
		}
	} else {
		return http_server::response::stock_reply(
		http_server::response::ok, "not yet implemented");
	}

}

boost::shared_ptr<Song> WebServer::GetSongFromJSON(std::string JsonDoc) {
	struct JsonSong {
	std::string edition;
	std::string title;
	std::string artist;
	std::string creator;
	std::string language;
	};
	JsonSong SongToFind {"","","","",""};

	if(JsonDoc[0] != '{') return boost::shared_ptr<Song>(); //check if someone did send the correct stuff.
	std::string currentIdentifier;
	bool ReadingIdentifier = false;
	bool IdentifierRead = false;
	bool ReadingContent = false;
	for(size_t i=0; i <= JsonDoc.length(); i++) {
		if(JsonDoc[i] == '}') {
			break;
		} else if(JsonDoc[i] == '\"') {
			if(IdentifierRead) {
			ReadingContent = true;
			IdentifierRead = false;
			} else if(ReadingIdentifier) {
			ReadingIdentifier = false;
			IdentifierRead = true;
			} else if(ReadingContent) {
			ReadingContent = false;
			currentIdentifier = "";
			} else {
			ReadingIdentifier = true;
			}
		} else {
			if(ReadingIdentifier) {
			currentIdentifier += JsonDoc[i];
			} else if (ReadingContent) {
			std::cout << currentIdentifier << std::endl;
				if(currentIdentifier == "Title") {
				SongToFind.title += JsonDoc[i];
				} else if (currentIdentifier == "Artist") {
				SongToFind.artist += JsonDoc[i];
				} else if (currentIdentifier == "Edition") {
				SongToFind.edition += JsonDoc[i];
				} else if (currentIdentifier == "Creator") {
				SongToFind.creator += JsonDoc[i];
				} else if (currentIdentifier == "Language") {
				SongToFind.language += JsonDoc[i];
				}
			}
		}
	}
	for(int i = 0; i<= m_songs.size(); i++) {
		if(m_songs[i].title == SongToFind.title && m_songs[i].artist == SongToFind.artist && m_songs[i].edition == SongToFind.edition &&
		m_songs[i].creator == SongToFind.creator && m_songs[i].language == SongToFind.language) { //if these are all correct we can assume it's the correct song
		boost::shared_ptr<Song> songToAdd(&m_songs[i]);
		return songToAdd;
		}
	}
	return NULL;
}
