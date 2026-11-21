#include "webserver.hh"

#include "game.hh"
#include "log.hh"
#include "platform.hh"

#ifdef USE_WEBSERVER
#include <boost/asio.hpp>

#include <chrono>
#include <string>
#include <thread>

using namespace std::chrono_literals;

void WebServer::StartServer(int tried, bool fallbackPortInUse) {
	if (tried > 2) {
		if (fallbackPortInUse == false) {
			SpdLogger::error(LogSystem::WEBSERVER, "Couldn't start webserver after 3 tries. Will now try fallback port.");
			// not changing this message for now because it's potentially translatable.
			m_game.notificationFromWebserver("Couldn't start webserver tried 3 times. Trying fallback port...");
			StartServer(0, true);
			return;
		}
		SpdLogger::error(LogSystem::WEBSERVER, "Couldn't start webserver after 3 tries, using the normal and fallback ports.");
		m_game.notificationFromWebserver("Couldn't start webserver.");
		return;
	}

	const int port = fallbackPortInUse ? static_cast<int>(config["game/webserver_fallback_port"].ui())
	                                   : static_cast<int>(config["game/webserver_port"].ui());
	std::string host;
	std::string displayAddr;
	if (config["game/webserver_access"].ui() == 1) {
		host = "127.0.0.1";
		displayAddr = "http://" + host + ":" + std::to_string(port);
		SpdLogger::notice(LogSystem::WEBSERVER, "Starting local server on IP={}", displayAddr);
	} else {
		// cpp-httplib binds sockets directly, so no http.sys/admin
		// privileges are required on Windows either.
		host = "0.0.0.0";
		displayAddr = "http://" + host + ":" + std::to_string(port);
		SpdLogger::notice(LogSystem::WEBSERVER, "Starting public server on IP={}", displayAddr);
	}

	try {
		m_server = std::make_shared<RequestHandler>(m_game, host, port, m_songs);
		std::string message = getIPaddr() + ":" + std::to_string(port);
		m_game.notificationFromWebserver(message);
		// listen() blocks until the server is stopped or binding fails.
		m_server->listen();
	} catch (std::exception& e) {
		tried = tried + 1;
		SpdLogger::error(LogSystem::WEBSERVER, "Exception={}. Trying again... (tried {} times).", e.what(), tried);
		std::string message = fmt::format("{}. Trying again... (tried {} times).", e.what(), tried);
		m_game.notificationFromWebserver(message);
		m_server.reset();
		std::this_thread::sleep_for(20s);
		StartServer(tried, fallbackPortInUse);
	}
}

WebServer::WebServer(Game &game, Songs& songs)
: m_game(game), m_songs(songs)
{
	if (config["game/webserver_access"].ui() == 0) {
		SpdLogger::notice(LogSystem::WEBSERVER, "Not starting webserver.");
	} else {
		m_serverThread = std::make_shared<std::thread>([this] { StartServer(0, false); });
	}
}

WebServer::~WebServer() {
	if (m_server) {
		try {
			m_server->stop();
		} catch (const std::exception& e) {
			SpdLogger::error(LogSystem::WEBSERVER, "Error stopping webserver. Exception={}.", e.what());
		}
	}
	if (m_serverThread && m_serverThread->joinable()) {
		m_serverThread->join();
	}
}

std::string WebServer::getIPaddr() {
	try {
		boost::asio::io_context netService;
		boost::asio::ip::udp::resolver resolver(netService);
		auto endpoints = resolver.resolve("1.1.1.1", "80");
		boost::asio::ip::udp::endpoint ep = endpoints.begin()->endpoint();
		boost::asio::ip::udp::socket socket(netService);
		socket.connect(ep);
		boost::asio::ip::address addr = socket.local_endpoint().address();
		return addr.to_string();
	} catch (std::exception& e) {
		std::string ip = boost::asio::ip::host_name();
		return ip.empty() ? "localhost" : ip;
	}
}
#endif
