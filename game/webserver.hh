#pragma once

#include <memory>
#include <string>
#include <thread>

#include "requesthandler.hh"

class WebServer {
public:
	WebServer(Songs& songs);
	~WebServer();

private:
	void StartServer(int tried, bool fallbackPortInUse);
	std::string getIPaddr();
	std::shared_ptr<std::thread> m_serverThread;
	std::shared_ptr<RequestHandler> m_server;
	Songs& m_songs;
};
