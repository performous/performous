#pragma once

class Game;
class Songs;

#ifdef USE_WEBSERVER
#include "requesthandler.hh"

#include <memory>
#include <thread>

class WebServer
{
public:
	WebServer(Game &game, Songs& songs);
	~WebServer();

private:
	std::shared_ptr<std::thread> m_serverThread;
	std::shared_ptr<RequestHandler> m_server;
	Game& m_game;
	void startServer(int tried, bool fallbackPortInUse);
	Songs& m_songs;
};
#else

class WebServer
{
public:
	WebServer(Game&, Songs&) {}
};
#endif
