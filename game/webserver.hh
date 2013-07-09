#pragma once
#include <boost/network/protocol/http/server.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <string>

using boost::thread;

namespace http = boost::network::http;

class WebServer
{
public:
struct handler;
typedef http::server<handler> http_server;
struct handler {
	void operator() (http_server::request const &request,
	http_server::response &response) {
		response = http_server::response::stock_reply(
		http_server::response::ok, "performous web-frontend!");
	}

	void log(http_server::string_type const &info) {
		std::cerr << "ERROR: " << info << '\n';
	}
};
	WebServer();
	~WebServer();
private:
	boost::scoped_ptr<boost::thread> serverthread;
	void StartServer();
};

