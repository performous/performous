#pragma once

class Songs;

#ifdef USE_WEBSERVER
#include "requesthandler.hh"
#include <memory>
#include <thread>

class WebServer
{
public:
	WebServer(Songs& songs);
	~WebServer();

private:
	void startServer(int tried, bool fallbackPortInUse);
	std::unique_ptr<std::thread> m_serverThread = nullptr;
	std::unique_ptr<RequestHandler> m_server = nullptr;
	Songs& m_songs;
};
#else

class WebServer
{
public:
	WebServer(Songs&) {}
};
#endif
