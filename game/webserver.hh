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
	void restartServer(); ///< Public interface to restart WebServer if configuration changes.
	
private:
	void startServer(int tried, bool fallbackPortInUse); ///< Start the WebServer.
	void stopServer(); ///< Stop the WebServer; called before restarting and in the destructor.
	std::unique_ptr<std::thread> m_serverThread = nullptr; ///< Thread responsible for initializing the RequestHandler.
	std::unique_ptr<RequestHandler> m_server = nullptr; ///< The actual server.
	Songs& m_songs; ///< Reference to the Songs database.
};
#else

class WebServer
{
public:
	WebServer(Songs&) {}
};
#endif
