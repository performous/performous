#include "webserver.hh"
#include <boost/network/protocol/http/server.hpp>

namespace http = boost::network::http;


void WebServer::StartServer() {
	handler handler_;
	http_server::options options(handler_);
	server_ = new http_server(options.address("127.0.0.1").port("8000"));
	server_->run();
}

WebServer::WebServer() {
	serverthread.reset(new boost::thread(boost::bind(&WebServer::StartServer,boost::ref(*this))));
}

WebServer::~WebServer() {
	server_->stop();
	serverthread->join();
	delete server_;
}
