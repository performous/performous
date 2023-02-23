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
	Game& m_game;
	void startServer(int tried, bool fallbackPortInUse);
	std::unique_ptr<std::thread> m_serverThread = nullptr;
	std::unique_ptr<RequestHandler> m_server = nullptr;
	Songs& m_songs;
};
#else

class WebServer
{
public:
	WebServer(Game&, Songs&) {}
};
#endif
