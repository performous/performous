#include "webserver.hh"
#include "game.hh"

#ifdef USE_WEBSERVER
#include <boost/asio.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/address_v4_range.hpp>

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

	unsigned short portToUse = fallbackPortInUse ? config["webserver/fallback_port"].ui() : config["webserver/port"].ui();
	std::string addr;
	std::string message("webserver/notice: Starting webserver on http://");
	if (config["webserver/access"].ui() == 1) {
		addr = "127.0.0.1";
		message = ", listening to connections from localhost";
	} else if (config["webserver/access"].ui() >= 2) {
		addr = "0.0.0.0";
		message += "; listening to any connections";
		
		if (config["webserver/access"].ui() == 3) {
			message += " originating from subnet " + config["webserver/subnet"].getValue();
		}
	}
	std::clog << message << "." << std::endl;
	try {
		m_server = std::shared_ptr<RequestHandler>(new RequestHandler(addr, m_songs));
// 		m_server->open().wait();
// 		std::string message = getIPaddr() + ":" +  std::to_string(portToUse);
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
	try {
		boost::asio::post(m_server->m_restinio_server.io_context(),
    [&] {
        // Starting the server in a sync way.
        m_server->m_restinio_server.open_sync();
		std::string message = getIPaddr() + ":" +  std::to_string(portToUse);
		m_game.notificationFromWebserver(message);
    });
		m_server->m_restinio_server.io_context().run();
	} catch (std::exception& e) {
		std::clog << "webserver/error: Failed to open RESTinio server due to: " << e.what() << ". Trying again... (tried " << tried << " times.)" << std::endl;
		std::string message(e.what());
		message += " Trying again... (tried " + std::to_string(tried) + " times).";
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
		std::clog << "webserver/debug: IP Address is: " << addr.to_string() << std::endl;
		auto testNetwork = boost::asio::ip::make_network_v4(addr.to_v4(), boost::asio::ip::make_address_v4("255.255.255.0"));
		std::string addresses;
		for (const auto& testAddress: testNetwork.hosts()) {
			if (!addresses.empty()) addresses += ", ";
			addresses += testAddress.to_string();
		}
		std::clog << "webserver/debug: Range of addresses is: " << addresses << std::endl;
		return addr.to_string();
	} catch(std::exception& e) {
		std::string ip = boost::asio::ip::host_name();
		return ip.empty() ? "localhost" : ip;
	}
}
#endif
