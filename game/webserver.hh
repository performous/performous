#pragma once

#ifdef USE_WEBSERVER

#include "requesthandler.hh"

class Game;

class WebServer
{
public:
	WebServer(Game &game, Songs& songs);
	~WebServer();

private:
	void StartServer(int tried, bool fallbackPortInUse);
	std::string getIPaddr();
	std::shared_ptr<std::thread> m_serverThread;
	std::shared_ptr<RequestHandler> m_server;
	Game& m_game;
	Songs& m_songs;
};
#else
class Songs;
class Game;

class WebServer
{
public:
	WebServer(Game&, Songs&) {}
};
#endif
