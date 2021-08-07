#include "webserver.hh"
#ifdef USE_WEBSERVER
#include "requesthandler.hh"
#include "screen.hh"
#include <boost/asio.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/address_v4_range.hpp>

void WebServer::startServer(int tried, bool fallbackPortInUse) {
	if(config["webserver/access"].i() == 0) {
		std::clog << "webserver/notice: Not starting webserver because it's been explicitly turned off." << std::endl;
		return;
	}
	if(tried > 2) {
			std::string message("webserver/error: Couldn't start webserver after 3 tries");
		if(fallbackPortInUse == false) {
			message += std::string("; trying fallback port...");
			std::clog << message << std::endl;
			Game::getSingletonPtr()->notificationFromWebserver(message);
			startServer(0, true);
			return;
		}
		message += std::string("using the main and fallback ports; won't try again.");
		std::clog << message << std::endl;
		Game::getSingletonPtr()->notificationFromWebserver(message);
	}

	unsigned short portToUse = fallbackPortInUse ? config["webserver/fallback_port"].i() : config["webserver/port"].i();
	std::string addr;
	std::string message("webserver/notice: Starting webserver, binding to ");
	if (config["webserver/access"].i() == 1) {
		addr = "127.0.0.1";
		message += addr;
		message += ", listening to connections from localhost";
	} else if (config["webserver/access"].i() >= 2) {
		addr = "0.0.0.0";
		message += addr;
		message += "; listening to any connections";
		if (config["webserver/access"].i() == 3) {
			message += " originating from subnet " + config["webserver/netmask"].getValue();
		}
		message += std::string(".");
	}
	std::clog << message << std::endl;
	try {
		m_server = std::make_unique<RequestHandler>(addr, portToUse, m_songs);
	} catch (std::exception& e) {
		tried = tried + 1;
		std::string message("webserver/error: " + std::string(e.what()) + ". Trying again... (tried " + std::to_string(tried) + " times).");
		std::clog << message << std::endl;
		Game::getSingletonPtr()->notificationFromWebserver(message);
		std::this_thread::sleep_for(20s);
		startServer(tried, fallbackPortInUse);
	}
	try {
		boost::asio::post(m_server->m_io_context, [&] {
				m_server->m_restinio_server->open_sync();
				std::string ip((config["webserver/access"].i() == 1) ? "localhost" : m_server->getLocalIP().to_string());
				std::string message("http://"+ip+":");
				message += std::to_string(portToUse);
				Game::getSingletonPtr()->notificationFromWebserver(message);
				});
		Performous_IP_Blocker::setAllowedSubnet(m_server->getLocalIP());
		m_server->m_io_context.run();
	} catch (std::exception& e) {
		std::string message("webserver/error: " + std::string(e.what()) + ". Trying again... (tried " + std::to_string(tried) + " times.)");
		std::clog << message << std::endl;
		Game::getSingletonPtr()->notificationFromWebserver(message);
		std::this_thread::sleep_for(20s);
		startServer(tried, fallbackPortInUse);
	}
}

WebServer::WebServer(Songs& songs)
: m_songs(songs)
{
	restartServer();
}

void WebServer::stopServer() {
	try {
		if (m_server) {
			m_server->m_restinio_server->close_sync();
			m_server->m_io_context.stop();
		} 
		if (m_serverThread) { m_serverThread->detach(); } // Using join results in a potential crash and/or locks-up forever waiting for the thread.
	} catch (const std::exception &e) {
		std::clog << "webserver/error: Failed to close RESTinio server due to: " << e.what() << "." << std::endl;
	}
}

void WebServer::restartServer() {
	stopServer();
	m_serverThread = std::make_unique<std::thread>([this] { startServer(0, false); });
}


WebServer::~WebServer() {
	stopServer();
}
#endif
