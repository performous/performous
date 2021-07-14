#include "webserver.hh"
#ifdef USE_WEBSERVER
#include "requesthandler.hh"
#include <boost/asio.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/address_v4_range.hpp>

void WebServer::StartServer(int tried, bool fallbackPortInUse) {
	if(tried > 2) {
		if(fallbackPortInUse == false) {
			std::clog << "webserver/error: Couldn't start webserver tried 3 times. Trying fallback port.." << std::endl;
			Game::getSingletonPtr()->notificationFromWebserver("Couldn't start webserver tried 3 times. Trying fallback port...");					
			StartServer(0, true);
			return;
		}

		std::clog << "webserver/error: Couldn't start webserver tried 3 times using normal port and 3 times using fallback port. Stopping webserver." << std::endl;
		Game::getSingletonPtr()->notificationFromWebserver("Couldn't start webserver.");							
		if(m_server) {
			m_server->close().wait();
		}
	}

	unsigned short portToUse = fallbackPortInUse ? config["webserver/fallback_port"].i() : config["webserver/port"].i();
	std::string addr;
	std::string message("webserver/notice: Starting webserver, binding to ");
	if (config["webserver/access"].i() == 1) {
		addr = "127.0.0.1";
		message += addr;
		message = ", listening to connections from localhost";
	} else if (config["webserver/access"].i() >= 2) {
		addr = "0.0.0.0";
		message += addr;
		message += "; listening to any connections";
		
		if (config["webserver/access"].i() == 3) {
			message += " originating from subnet " + config["webserver/netmask"].getValue();
		}
	}
	std::clog << message << "." << std::endl;
	try {
		m_server = std::shared_ptr<RequestHandler>(new RequestHandler(addr, portToUse, m_songs));
	} catch (std::exception& e) {
		tried = tried + 1;
		std::clog << "webserver/error: " << e.what() << ". Trying again... (tried " << tried << " times)." << std::endl;
		std::string message(e.what());
		message += ". Trying again... (tried " + std::to_string(tried) +" times).";
		Game::getSingletonPtr()->notificationFromWebserver(message);		
		std::this_thread::sleep_for(20s);
		StartServer(tried, fallbackPortInUse);
	}
	try {
		boost::asio::post(m_server->m_restinio_server.io_context(),
    [&] {
        // Starting the server in a sync way.
        m_server->m_restinio_server.open_sync();
		std::string message = m_server->getLocalIP().to_string() + ":" +  std::to_string(portToUse);
		Game::getSingletonPtr()->notificationFromWebserver(message);
    });
    	Performous_IP_Blocker::setAllowedSubnet(m_server->getLocalIP());
		m_server->m_restinio_server.io_context().run();
	} catch (std::exception& e) {
		std::clog << "webserver/error: Failed to open RESTinio server due to: " << e.what() << ". Trying again... (tried " << tried << " times.)" << std::endl;
		std::string message(e.what());
		message += " Trying again... (tried " + std::to_string(tried) + " times).";
		Game::getSingletonPtr()->notificationFromWebserver(message);		
		std::this_thread::sleep_for(20s);
		StartServer(tried, fallbackPortInUse);
	}
}

WebServer::WebServer(Songs& songs)
: m_songs(songs)
{
	if(config["webserver/access"].i() == 0) {
		std::clog << "webserver/notice: Not starting webserver." << std::endl;
	} else {
		m_serverThread = std::make_unique<std::thread>([this] { StartServer(0, false); });
	}
}

WebServer::~WebServer() {
	if( m_server ) {
// 		m_server->close().wait();
		try {
			m_server->m_restinio_server.close_sync();
			m_server->m_restinio_server.io_context().stop();
		} catch (const std::exception &e) {
			std::clog << "webserver/error: Failed to close RESTinio server due to: " << e.what() << "." << std::endl;
		}
		m_serverThread->join();
	}
}
#endif
