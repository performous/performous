#pragma once
#include <boost/network/protocol/http/server.hpp>
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

namespace http = boost::network::http;

class WebServer
{
public:
	struct handler;
	typedef http::server<handler> http_server;

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
};

