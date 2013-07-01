#pragma once
#include <boost/network/protocol/http/server.hpp>
#include <string>

namespace http = boost::network::http;

struct handler;
typedef http::server<handler> http_server;
struct handler {
	void operator() (http_server::request const &request,
	http_server::response &response) {
		response = http_server::response::stock_reply(
		http_server::response::ok, "Hello, world!");
	}

	void log(http_server::string_type const &info) {
		std::cerr << "ERROR: " << info << '\n';
	}
};


class WebServer
{
public:
	WebServer();
private:
};

