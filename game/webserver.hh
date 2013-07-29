#pragma once
#include <boost/network/protocol/http/server.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include "fs.hh"
#include "songs.hh"
#include "song.hh"
#include "screen.hh"
#include "playlist.hh"

using boost::thread;

namespace http = boost::network::http;

class WebServer
{
public:
struct handler;
typedef http::server<handler> http_server;
	WebServer(Songs& songs);
	~WebServer();
private:
	boost::scoped_ptr<boost::thread> serverthread;
	http_server* server_; //FIXME should be boost::scoped_ptr or boost:: shared_ptr
public:
	Songs& m_songs;
	//this looks silly but this stuf has to be public or they won´t be accessible from the handler struct
	http_server::response GETresponse(http_server::request const &request);
	http_server::response POSTresponse(http_server::request const &request);
private:
	void StartServer();
	boost::shared_ptr<Song> GetSongFromJSON(std::string JsonDoc);
};

