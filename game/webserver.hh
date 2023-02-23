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
	WebServer(WebServer&) = delete;
	~WebServer() = default;
	void restartServer(); ///< Public interface to restart WebServer if configuration changes.
	void stopServer(); ///< Stop the WebServer; called before restarting the webserver, and when quitting Performous.
	
private:
	Game& m_game;
	void startServer(int tried, bool fallbackPortInUse); ///< Start the WebServer.
	std::unique_ptr<std::thread> m_serverThread = nullptr; ///< Thread responsible for initializing the RequestHandler.
	std::unique_ptr<RequestHandler> m_server = nullptr; ///< The actual server.
	Songs& m_songs; ///< Reference to the Songs database.
	std::unique_ptr<boost::asio::io_context> m_io_context = nullptr; ///< ASIO io_context that holds the webserver logic.
};

#else

class WebServer
{
public:
	WebServer(Game&, Songs&) {}
};
#endif
