#include "webserver.hh"
#ifdef USE_WEBSERVER
#include "i18n.hh"
#include "requesthandler.hh"
#include "screen.hh"

#include <boost/asio.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/address_v4_range.hpp>

void WebServer::startServer(int tried, bool fallbackPortInUse) {
	std::string message(_("Webserver active!\nConnect to this computer\nusing "));
	if(config["webserver/access"].i() == 0) {
		std::clog << "webserver/notice: Not starting webserver because it's been explicitly turned off." << std::endl;
		return;
	}
	if(tried > 2) {
		message = _("Couldn't start webserver");
		if(fallbackPortInUse == false) {
			message += _("; trying fallback port...");
			std::clog << message << std::endl;
			Game::getSingletonPtr()->notificationFromWebserver(message);
			startServer(0, true);
			return;
		}
		message += _("on either port; will now disable it.");
		std::clog << "webserver/warning: " << message << std::endl;
		Game::getSingletonPtr()->notificationFromWebserver(_("Webserver Error:") + " " + message);
	}
	unsigned short portToUse = fallbackPortInUse ? config["webserver/fallback_port"].i() : config["webserver/port"].i();
	std::string addr;
	std::string logMsg("webserver/notice: Starting webserver, binding to ");
	if (config["webserver/access"].i() == 1) {
		addr = "127.0.0.1";
		logMsg += addr;
		logMsg += ", listening to connections from localhost";
	} else if (config["webserver/access"].i() >= 2) {
		addr = "0.0.0.0";
		logMsg += addr;
		logMsg += std::string("; listening to any connections");
		if (config["webserver/access"].i() == 3) {
			logMsg += std::string(" originating from subnet ") += config["webserver/netmask"].getValue();
		}
		logMsg += std::string(".");
	}
	std::clog << logMsg << std::endl;
	try {
		m_server = std::make_unique<RequestHandler>(addr, portToUse, m_songs);
	} catch (std::exception& e) {
		++tried;
		message = std::string(e.what() + std::string(". \n"));
		message += _("Trying again... (tried ") += std::to_string(tried) += std::string(" times).");
		std::clog << "webserver/error: " << message << std::endl;
;
		Game::getSingletonPtr()->notificationFromWebserver(_("Webserver Error:") + " " + message);
		std::this_thread::sleep_for(20s);
		startServer(tried, fallbackPortInUse);
	}
	try {
		boost::asio::post(m_server->m_io_context, [&] {
				m_server->m_restinio_server->open_sync();
				std::string ip((config["webserver/access"].i() == 1) ? "localhost" : m_server->getLocalIP().to_string());
				message += std::string("http://") += ip += std::string(":") += std::to_string(portToUse);
				Game::getSingletonPtr()->notificationFromWebserver(message);
				});
		Performous_IP_Blocker::setAllowedSubnet(m_server->getLocalIP());
		m_server->m_io_context.run();
	} catch (std::exception& e) {
		++tried;
		message = std::string(e.what() + std::string(". \n"));
		message += _("Trying again... (tried ") += std::to_string(tried) += std::string(" times).");
		std::clog << "webserver/error: " << message << std::endl;
		Game::getSingletonPtr()->notificationFromWebserver(_("Webserver Error:") + " " + message);
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
