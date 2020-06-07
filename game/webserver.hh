#pragma once

#ifdef USE_WEBSERVER

#include "requesthandler.hh"
class Songs;

class WebServer
{
public:
	WebServer(Songs& songs);
	~WebServer();

private:
	std::shared_ptr<std::thread> m_serverThread;
	std::shared_ptr<RequestHandler> m_server;
	void startServer(int tried, bool fallbackPortInUse);
	Songs& m_songs;
};
#else
class Songs;

class WebServer
{
public:
	WebServer(Songs&) {}
};
#endif
