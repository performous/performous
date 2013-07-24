#include "webserver.hh"
#include <boost/network/protocol/http/server.hpp>

namespace http = boost::network::http;

struct WebServer::handler {
	void operator() (http_server::request const &request,
	http_server::response &response) {
	//destination describes the file to be loaded, by default it's "/"
		if(request.method == "GET") {
			if(request.destination == "/") {
			fs::ifstream f(findFile("index.html"), std::ios::binary);
			f.seekg(0, std::ios::end);
			size_t size = f.tellg();
			f.seekg(0);
			char responseBuffer[size + 2];
			f.read(responseBuffer, size);
			response = http_server::response::stock_reply(
			http_server::response::ok, responseBuffer);
			} else if(request.destination == "/codiqa.ext.js"){
					fs::ifstream f(findFile("codiqa.ext.js"), std::ios::binary);
					f.seekg(0, std::ios::end);
					size_t size = f.tellg();
					f.seekg(0);
					char responseBuffer[size + 2];
					f.read(responseBuffer, size);
					response = http_server::response::stock_reply(
					http_server::response::ok, responseBuffer);
			} else if(request.destination == "/codiqa.ext.css") {
					fs::ifstream f(findFile("codiqa.ext.css"), std::ios::binary);
					f.seekg(0, std::ios::end);
					size_t size = f.tellg();
					f.seekg(0);
					char responseBuffer[size + 2];
					f.read(responseBuffer, size);
					response = http_server::response::stock_reply(
					http_server::response::ok, responseBuffer);
			} else {
					//other files
				}
		} else if(request.method == "POST") {
		response = http_server::response::stock_reply(
			http_server::response::ok, "Post request");
		} else {
		response = http_server::response::stock_reply(
			http_server::response::ok, "other request");
		}
	}

	void log(http_server::string_type const &info) {
		std::cerr << "ERROR: " << info << '\n';
	}
};

void WebServer::StartServer() {
	handler handler_;
	http_server::options options(handler_);
	server_ = new http_server(options.address("0.0.0.0").port("8000"));
	server_->run();
}

WebServer::WebServer(Database& db):
m_database(db){
	serverthread.reset(new boost::thread(boost::bind(&WebServer::StartServer,boost::ref(*this))));
}

WebServer::~WebServer() {
	server_->stop();
	serverthread->join();
	delete server_;
}
