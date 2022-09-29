#include "webserver.hh"
#include "game.hh"

#ifdef USE_WEBSERVER
#include <boost/asio.hpp>

void WebServer::StartServer(int tried, bool fallbackPortInUse) {
	if(tried > 2) {
		if(fallbackPortInUse == false) {
			std::clog << "webserver/error: Couldn't start webserver tried 3 times. Trying fallback port.." << std::endl;
			m_game.notificationFromWebserver("Couldn't start webserver tried 3 times. Trying fallback port...");
			StartServer(0, true);
			return;
		}

		std::clog << "webserver/error: Couldn't start webserver tried 3 times using normal port and 3 times using fallback port. Stopping webserver." << std::endl;
		m_game.notificationFromWebserver("Couldn't start webserver.");
		if(m_server) {
			m_server->close().wait();
		}
	}

	std::string portToUse = fallbackPortInUse ? std::to_string(config["game/webserver_fallback_port"].ui()) : std::to_string(config["game/webserver_port"].ui());
	std::string addr("");
	if(config["game/webserver_access"].ui() == 1) {
		addr = "http://127.0.0.1:" + portToUse;
		std::clog << "webserver/notice: Starting local server on: " << addr <<std::endl;
	} else {
		addr = "http://0.0.0.0:" + portToUse;
		std::clog << "webserver/notice: Starting public server on: " << addr << std::endl;
	}

	try {
		m_server = std::shared_ptr<RequestHandler>(new RequestHandler(m_game, addr, m_songs));
		m_server->open().wait();
		std::string message = getIPaddr() + ":" +  portToUse;
		m_game.notificationFromWebserver(message);
	} catch (std::exception& e) {
		tried = tried + 1;
		std::clog << "webserver/error: " << e.what() << " Trying again... (tried " << tried << " times)." << std::endl;
		std::string message(e.what());
		message += " Trying again... (tried " + std::to_string(tried) +" times).";
		m_game.notificationFromWebserver(message);
		std::this_thread::sleep_for(20s);
		StartServer(tried, fallbackPortInUse);
	}
}

WebServer::WebServer(Game &game, Songs& songs)
: m_game(game), m_songs(songs)
{
	if(config["game/webserver_access"].ui() == 0) {
		std::clog << "webserver/notice: Not starting webserver." << std::endl;
	} else {
		m_serverThread = std::make_unique<std::thread>([this] { StartServer(0, false); });
	}
}

WebServer::~WebServer() {
	if( m_server ) {
            try {
		m_server->close().wait();
		m_serverThread->join();
            } catch (const pplx::invalid_operation &e) {
                std::clog << "webserver/error: stoping webserver failed: " << e.what() << std::endl;
            }
	}
}

std::string WebServer::getIPaddr() {
	try {
		boost::asio::io_service netService;
		boost::asio::ip::udp::resolver resolver(netService);
		boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), "1.1.1.1", "80");
		boost::asio::ip::udp::resolver::iterator endpoints = resolver.resolve(query);
		boost::asio::ip::udp::endpoint ep = *endpoints;
		boost::asio::ip::udp::socket socket(netService);
		socket.connect(ep);
		boost::asio::ip::address addr = socket.local_endpoint().address();
		return addr.to_string();
	} catch(std::exception& e) {
		std::string ip = boost::asio::ip::host_name();
		return ip.empty() ? "localhost" : ip;
	}
}
#endif
