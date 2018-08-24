#include "webserver.hh"
#ifdef USE_WEBSERVER

void WebServer::StartServer(int tried, bool fallbackPortInUse) {
	if(tried > 2) {
		if(fallbackPortInUse == false) {
			std::clog << "webserver/error: Couldn't start webserver tried 3 times. Trying fallbackport.." << std::endl;
			StartServer(0, true);
			return;
		}

		std::clog << "webserver/error: Couldn't start webserver tried 3 times using normal port and 3 times using fallback port. Stopping webserver." << std::endl;
		if(m_server) {
			m_server->close().wait();
		}
	}

	std::string portToUse = fallbackPortInUse ? std::to_string(config["game/webserver_fallback_port"].i()) : std::to_string(config["game/webserver_port"].i());
	std::string addr("");
	if(config["game/webserver_access"].i() == 1) {
		addr = "http://127.0.0.1:" + portToUse;
		std::clog << "webserver/notice: Starting local server on: " << addr <<std::endl;
	} else {
		addr = "http://0.0.0.0:" + portToUse;
		std::clog << "webserver/notice: Starting public server on: " << addr << std::endl;
	}

	try {
		m_server = std::shared_ptr<RequestHandler>(new RequestHandler(addr, m_songs));
		m_server->open().wait();
	} catch (std::exception& e) {
		tried = tried + 1;
		std::clog << "webserver/error: " << e.what() << " Trying again... (tried " << tried << " times)." << std::endl;
		std::this_thread::sleep_for(20s);
		StartServer(tried, fallbackPortInUse);
	}
}

WebServer::WebServer(Songs& songs)
: m_songs(songs)
{
	if(config["game/webserver_access"].i() == 0) {
		std::clog << "webserver/notice: Not starting webserver." << std::endl;
	} else {
		m_serverThread = std::make_unique<std::thread>([this] { StartServer(0, false); });
	}
}

WebServer::~WebServer() {
	if( m_server ) {
		m_server->close().wait();
		m_serverThread->join();
	}
}
#endif