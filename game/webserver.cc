#include "webserver.hh"

#include "game.hh"
#include "log.hh"
#include "platform.hh"

#ifdef USE_WEBSERVER
#include <boost/asio.hpp>

void WebServer::StartServer(int tried, bool fallbackPortInUse) {
	if(tried > 2) {
		if(fallbackPortInUse == false) {
			SpdLogger::error(LogSystem::WEBSERVER, "Couldn't start webserver after 3 tries. Will now try fallback port.");
			// not changing this message for now because it's potentially translatable. 
			m_game.notificationFromWebserver("Couldn't start webserver tried 3 times. Trying fallback port...");
			StartServer(0, true);
			return;
		}
		SpdLogger::error(LogSystem::WEBSERVER, "Couldn't start webserver after 3 tries, using the normal and fallback ports.");
		m_game.notificationFromWebserver("Couldn't start webserver.");
		if(m_server) {
			m_server->close().wait();
		}
	}

	std::string portToUse = fallbackPortInUse ? std::to_string(config["game/webserver_fallback_port"].ui()) : std::to_string(config["game/webserver_port"].ui());
	std::string addr("");
	if(config["game/webserver_access"].ui() == 1) {
		addr = "http://127.0.0.1:" + portToUse;
		SpdLogger::notice(LogSystem::WEBSERVER, "Starting local server on IP={}", addr);
	} else {
		if (Platform::currentOS() == Platform::HostOS::OS_WIN)
		{
			addr = "http://+:" + portToUse; // Allow Windows to accept all connections. Needs admin privileges though.
		}
		else 
		{
			addr = "http://0.0.0.0:" + portToUse;
		}
		SpdLogger::notice(LogSystem::WEBSERVER, "Starting public server on IP={}", addr);
	}

	try {
		m_server = std::shared_ptr<RequestHandler>(new RequestHandler(m_game, addr, m_songs));
		m_server->open().wait();
		std::string message = getIPaddr() + ":" +  portToUse;
		m_game.notificationFromWebserver(message);
	} catch (std::exception& e) {
		tried = tried + 1;
		SpdLogger::error(LogSystem::WEBSERVER, "Exception={}. Trying again... (tried {} times).", e.what(), tried);
		std::string message = fmt::format("{}. Trying again... (tried {} times).", e.what(), tried);
		m_game.notificationFromWebserver(message);
		std::this_thread::sleep_for(20s);
		StartServer(tried, fallbackPortInUse);
	}
}

WebServer::WebServer(Game &game, Songs& songs)
: m_game(game), m_songs(songs)
{
	if(config["game/webserver_access"].ui() == 0) {
		SpdLogger::notice(LogSystem::WEBSERVER, "Not starting webserver.");
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
            	SpdLogger::error(LogSystem::WEBSERVER, "Error stopping webserver. Exception={}.", e.what());
            }
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
	} catch(std::exception& e) {
		std::string ip = boost::asio::ip::host_name();
		return ip.empty() ? "localhost" : ip;
	}
}
#endif
