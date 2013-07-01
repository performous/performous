#include "webserver.hh"
namespace http = boost::network::http;

WebServer::WebServer() {
	handler handler_;
	http_server::options options(handler_);
	http_server server_(
	options.address("127.0.0.1").port("8000"));
	std::cout << "code hit!!" << std::endl;
	server_.run();
	std::cout << "server running!" << std::endl;
}
