#pragma once

#ifdef USE_CPPNETLIB
	#include <boost/network/protocol/http/server.hpp>
	namespace http = boost::network::http;
#endif
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include "fs.hh"
#include "songs.hh"
#include "song.hh"
#include "screen.hh"
#include "playlist.hh"
#include "screen_playlist.hh"
#include "controllers.hh"
#include "configuration.hh"

using boost::thread;

class WebServer
{
#ifdef USE_CPPNETLIB
public:
	struct handler;
	typedef http::server<handler> http_server;
	typedef std::vector<std::uint8_t> BinaryBuffer;
	BinaryBuffer readFile(fs::path const& path) {
		BinaryBuffer ret;
		fs::ifstream f(path, std::ios::binary);
		f.seekg(0, std::ios::end);
		ret.resize(f.tellg());
		f.seekg(0);
		f.read(reinterpret_cast<char*>(ret.data()), ret.size());
		if (!f) throw std::runtime_error("File cannot be read: " + path.string());
		return ret;
	}
	WebServer(Songs& songs);
	~WebServer();
	// This looks silly but this stuff has to be public or they won't be accessible from the handler struct
	http_server::response GETresponse(http_server::request const &request);
	http_server::response POSTresponse(http_server::request const &request);

private:
	void StartServer();
	boost::shared_ptr<Song> GetSongFromJSON(std::string JsonDoc);
	std::string escapeCharacters(std::string input);
	std::string unEscapeCharacters(std::string input);
	std::string ReplaceCharacters(std::string input, std::string search, std::string replace);
	boost::shared_ptr<boost::thread> m_serverThread;
	boost::shared_ptr<http_server> m_server;
	Songs& m_songs;
#else
public:
	WebServer(Songs& songs);
	~WebServer();
private:
	Songs& m_songs;
#endif
};

