#pragma once
#include <boost/network/protocol/http/server.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem/fstream.hpp>
#include <string>
#include "fs.hh"
#include "database.hh"

using boost::thread;

namespace http = boost::network::http;

class WebServer
{
public:
struct handler;
typedef http::server<handler> http_server;

	WebServer(Database& db);
	~WebServer();
private:
	boost::scoped_ptr<boost::thread> serverthread;
	void StartServer();
	http_server* server_; //FIXME should be boost::scoped_ptr or boost:: shared_ptr
	Database& m_database;
};

