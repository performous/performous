#include "webserver.hh"
#include <boost/network/protocol/http/server.hpp>

namespace http = boost::network::http;

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


void StartServer() {
	handler handler_;
	http_server::options options(handler_);
	http_server server_(
	options.address("127.0.0.1").port("8000"));
	server_.run();
}

WebServer::WebServer()
	:serverthread(&StartServer) {
	serverthread.start_thread();
}
