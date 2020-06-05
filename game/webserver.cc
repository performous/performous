#include "webserver.hh"
#include "game.hh"

#ifdef USE_WEBSERVER
#include "requesthandler.hh"
#include <boost/asio.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/address_v4_range.hpp>

void WebServer::StartServer(int tried, bool fallbackPortInUse) {
	if(tried > 2) {
			std::string message("webserver/error: Couldn't start webserver after 3 tries");
		if(fallbackPortInUse == false) {
		 	message += std::string("; trying fallback port...");
			std::clog << message << std::endl;
			m_game.notificationFromWebserver("Couldn't start webserver tried 3 times. Trying fallback port...");
			StartServer(0, true);
			return;
		}
		message += std::string("using the main and fallback ports; won't try again.");
		std::clog << message << std::endl;
		m_game.notificationFromWebserver("Couldn't start webserver.");
		}

	unsigned short portToUse = fallbackPortInUse ? config["webserver/fallback_port"].ui() : config["webserver/port"].ui();
	std::string addr;
	std::string message("webserver/notice: Starting webserver, binding to ");
	if (config["webserver/access"].ui() == 1) {
		addr = "127.0.0.1";
		message += addr;
		message += ", listening to connections from localhost";
	} else if (config["webserver/access"].ui() >= 2) {
		addr = "0.0.0.0";
		message += addr;
		message += "; listening to any connections";
		if (config["webserver/access"].ui() == 3) {
			message += " originating from subnet " + config["webserver/netmask"].getValue();
		}
		message += std::string(".");
	}
	std::clog << message << std::endl;
	try {
		m_server = std::shared_ptr<RequestHandler>(new RequestHandler(addr, portToUse, m_songs));
		m_game.notificationFromWebserver(message);
	} catch (std::exception& e) {
		tried = tried + 1;
		std::string message("webserver/error: " + std::string(e.what()) + ". Trying again... (tried " + std::to_string(tried) + " times)."); 
		std::clog << message << std::endl;
		m_game.notificationFromWebserver(message);
		std::this_thread::sleep_for(20s);
		StartServer(tried, fallbackPortInUse);
	}
	try {
		boost::asio::post(m_server->m_restinio_server.io_context(), [&] {
        		m_server->m_restinio_server.open_sync();
				std::string message(m_server->getLocalIP().to_string()+":");
				message += std::to_string(portToUse);
		m_game.notificationFromWebserver(message);
				});
    	Performous_IP_Blocker::setAllowedSubnet(m_server->getLocalIP());
		m_server->m_restinio_server.io_context().run();
	} catch (std::exception& e) {
		std::string message("webserver/error: " + std::string(e.what()) + ". Trying again... (tried " + std::to_string(tried) + " times.)");
		std::clog << message << std::endl;
		m_game.notificationFromWebserver(message);
		std::this_thread::sleep_for(20s);
		StartServer(tried, fallbackPortInUse);
	}
}

WebServer::WebServer(Game &game, Songs& songs)
: m_game(game), m_songs(songs)
{
	if(config["webserver/access"].ui() == 0) {
		std::clog << "webserver/notice: Not starting webserver." << std::endl;
	} else {
		m_serverThread = std::make_unique<std::thread>([this] { StartServer(0, false); });
	}
}

WebServer::~WebServer() {
	try {
		if (m_server) {
			m_server->m_restinio_server.close_sync();
			m_server->m_restinio_server.io_context().stop();
		} 
		if (m_serverThread) { m_serverThread->detach(); } // Using join results in a potential crash and/or locks-up forever waiting for the thread.
	} catch (const std::exception &e) {
		std::clog << "webserver/error: Failed to close RESTinio server due to: " << e.what() << "." << std::endl;
		}
}
#endif
