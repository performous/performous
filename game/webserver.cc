#include "webserver.hh"
#include <boost/network/protocol/http/server.hpp>

namespace http = boost::network::http;

typedef http::server<WebServer::handler> http_server;

struct WebServer::handler {
	WebServer& m_server;
	void operator() (http_server::request const &request,
	http_server::response &response) {
		//destination describes the file to be loaded, by default it's "/"
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
		std::cerr << "ERROR: " << info << '\n';
	}
};


void WebServer::StartServer() {
	handler handler_{*this};
	http_server::options options(handler_);
	server_.reset(new http_server(options.address("0.0.0.0").port("8000")));
	server_->run();
}

WebServer::WebServer(Songs& songs):
m_songs(songs) {
	serverthread.reset(new boost::thread(boost::bind(&WebServer::StartServer,boost::ref(*this))));
}

WebServer::~WebServer() {
	server_->stop();
	serverthread->join();
}

http_server::response WebServer::GETresponse(const http_server::request &request) {
	if (request.destination == "/") { //default
		BinaryBuffer buf = readFile(findFile("index.html"));
		return http_server::response::stock_reply(http_server::response::ok, std::string(buf.begin(), buf.end()));
	} else if (request.destination == "/api/getDataBase.json") { //get database
		std:: stringstream JSONDB;
		JSONDB << "[\n";
		for (int i=0; i<m_songs.size(); i++) {
			JSONDB << "\n{\n\"Title\": \"" << escapeCharacters(m_songs[i]->title) << "\",\n\"Artist\": \"";
			JSONDB << escapeCharacters(m_songs[i]->artist) << "\",\n\"Edition\": \"" << escapeCharacters(m_songs[i]->edition) << "\",\n\"Language\": \"" << escapeCharacters(m_songs[i]->language);
			JSONDB << "\",\n\"Creator\": \"" << escapeCharacters(m_songs[i]->creator) << "\"\n},";
		}
		std::string output = JSONDB.str(); //remove the last comma
		output.pop_back(); //remove the last comma
		output += "\n]";
		return http_server::response::stock_reply(
		http_server::response::ok, output);
	} else if (request.destination == "/api/getCurrentPlaylist.json") { //get playlist
		Game* gm = Game::getSingletonPtr();
		std:: stringstream JSONPlayList;
		JSONPlayList << "[\n";
		for (auto const& song : gm->getCurrentPlayList().getList()) {
			JSONPlayList << "\n{\n\"Title\": \"" << escapeCharacters(song->title) << "\",\n\"Artist\": \"";
			JSONPlayList << escapeCharacters(song->artist) << "\",\n\"Edition\": \"" << escapeCharacters(song->edition) << "\",\n\"Language\": \"" << escapeCharacters(song->language);
			JSONPlayList << "\",\n\"Creator\": \"" << escapeCharacters(song->creator) << "\"\n},";
		}
		std::string output = JSONPlayList.str();
		output.pop_back(); //remove the last comma
		output += "\n]";
		return http_server::response::stock_reply(http_server::response::ok, output);
	} else {
		//other text files
		try {
			std::string destination = request.destination;
			///this is to make sure the tree is only accessible downwards
			while(destination[1] == '.' && destination[2] == '.' && destination[3] == '//') {
				destination.erase(1,4);
			}
			destination.erase(0,1);//remove the first /
			BinaryBuffer buf = readFile(findFile(destination));
			return http_server::response::stock_reply(http_server::response::ok, std::string(buf.begin(), buf.end()));
		}
		catch(std::exception e) {
			return http_server::response::stock_reply(
			http_server::response::ok, "not a text file");
		}
	}
}

http_server::response WebServer::POSTresponse(const http_server::request &request) {
	Game * gm = Game::getSingletonPtr();
	if (request.destination == "/api/add") {
		boost::shared_ptr<Song> pointer = GetSongFromJSON(request.body);
		if (!pointer) {
			return http_server::response::stock_reply(
			http_server::response::ok, "failure");
		} else {
			std::cout << pointer->title << std::endl;
			gm->getCurrentPlayList().addSong(pointer);
			ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
			m_pp->triggerSongListUpdate(); //this way the screen_playlist does a live-update just like the screen_songs
			return http_server::response::stock_reply(
			http_server::response::ok, "success");
		}
	} else if(request.destination == "/api/remove") {
		try { // this is for those idiots that send text instead of numbers.
			int songToDelete = boost::lexical_cast<int>(request.body);
			gm->getCurrentPlayList().removeSong(songToDelete);
			ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(gm->getScreen("Playlist"));
			m_pp->triggerSongListUpdate(); //this way the screen_playlist does a live-update just like the screen_songs
			return http_server::response::stock_reply(
			http_server::response::ok, "success");
		} catch(std::exception e) {
			return http_server::response::stock_reply(
			http_server::response::ok, "failure");
		}
	} else {
		return http_server::response::stock_reply(
		http_server::response::ok, "not yet implemented");
	}
}

boost::shared_ptr<Song> WebServer::GetSongFromJSON(std::string JsonDoc) {
	// DONE: Implement operator== here, to avoid repeating very long if (...) everywhere where comparisons are needed.
	struct JsonSong {
		std::string edition;
		std::string title;
		std::string artist;
		std::string creator;
		std::string language;
		bool operator==(JsonSong rhs) {
			if(this->title == rhs.title && this->edition == rhs.edition && this->artist == rhs.artist && this->creator == rhs.creator && this->language == rhs.language) {
				return true;
			} else {
				return false;
			}
		}
	};
	JsonSong SongToFind;

	if (JsonDoc[0] != '{') return boost::shared_ptr<Song>(); //check if someone did send the correct stuff.

	std::string currentIdentifier;
	bool ReadingIdentifier = false;
	bool IdentifierRead = false;
	bool ReadingContent = false;
	for (size_t i=0; i <= JsonDoc.length(); i++) {
		if (JsonDoc[i] == '}') {
			break;
		} else if (JsonDoc[i] == '\"' &&JsonDoc[i-1] != '\\' ) { //make sure it's not an escaped character :-)
			if (IdentifierRead) {
				ReadingContent = true;
				IdentifierRead = false;
			} else if (ReadingIdentifier) {
				ReadingIdentifier = false;
				IdentifierRead = true;
			} else if (ReadingContent) {
				ReadingContent = false;
				currentIdentifier = "";
			} else {
				ReadingIdentifier = true;
			}
		} else {
			if (ReadingIdentifier) {
				currentIdentifier += JsonDoc[i];
			} else if (ReadingContent) {
				std::cout << currentIdentifier << std::endl;
				if (currentIdentifier == "Title") {
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
	//remove the json escape chars
	SongToFind.title = unEscapeCharacters(SongToFind.title);
	SongToFind.artist = unEscapeCharacters(SongToFind.artist);
	SongToFind.edition = unEscapeCharacters(SongToFind.edition);
	SongToFind.creator = unEscapeCharacters(SongToFind.creator);
	SongToFind.language = unEscapeCharacters(SongToFind.language);


	Game* gm = Game::getSingletonPtr();
	for (int i = 0; i<= m_songs.size(); i++) {
		///this is to fix the crash when adding the currently-playing song.
		if (gm->getCurrentPlayList().currentlyActive) {
			Song const& s = *gm->getCurrentPlayList().currentlyActive;
			if (s.title == SongToFind.title && s.artist == SongToFind.artist &&
			  s.creator == SongToFind.creator && s.edition == SongToFind.edition &&
			  s.language == SongToFind.language)
			{
				return gm->getCurrentPlayList().currentlyActive;
			}
		}
		///this is for all other songs.
		boost::shared_ptr<Song> s = m_songs[i];
		JsonSong m_compare = {s->edition, s->title, s->artist, s->creator, s->language};

		if (SongToFind == m_compare) {
			return s;
		}
	}
	return boost::shared_ptr<Song>();
}

std::string WebServer::escapeCharacters(std::string input) {
	size_t pos = 0;
	std::string search = "\"";
	std::string replace = "\\\"";
	while ((pos = input.find(search, pos)) != std::string::npos) {
		input.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return input;
}

std::string WebServer::unEscapeCharacters(std::string input) {
	size_t pos = 0;
	std::string search = "\\\"";
	std::string replace = "\"";
	while ((pos = input.find(search, pos)) != std::string::npos) {
		input.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return input;
}

