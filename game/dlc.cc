///This file contains implementation of dlc class
#include "dlc.hh"

#include <string>
#include <fstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <fstream>
#include <vector>
#include <boost/asio.hpp>

Dlc::Dlc(DlcConfig dlcConfig){}
Dlc::Dlc(){}
Dlc::~Dlc(){}

std::string url = "localhost/catalog.xml";
std::string hostname = "localhost";
std::string filename = "/catalog.xml";

// below commented code  
//#define BOOST_THREAD_USE_LIB
/*
namespace {
std::string get_filename(const uri::uri &url) {
    std::string path = uri::path(url);
    std::size_t index = path.find_last_of('/');
    std::string filename = path.substr(index + 1);
    return filename.empty()? "index.html" : filename;
}
*/
/*
void Dlc::getTheCatalog() {

#include <boost/network/protocol/http/client.hpp>
#include <boost/network/uri.hpp>


	namespace http = boost::network::http;
	namespace uri = boost::network::uri;

    try {
        http::client client;
        http::client::request request(url);
        http::client::response response = client.get(request);

        //std::string filename = get_filename(request.uri());
        std::string filename = "catalog12.xml";
        std::cout << "Saving to: " << filename << std::endl;
        std::ofstream ofs(filename.c_str());
        ofs << static_cast<std::string>(body(response)) << std::endl;
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        //return 1;
    }
}
*/

std::string outputFilename = "catalog.xml";
std::string port = "80";
unsigned int timeout = 800;
	
/// This function try to get the song catalog from server using boost asio 	
void Dlc::getCatalog(){
	
	std::ofstream out_(outputFilename);
	std::vector<std::string> headers;

	try{
        using namespace boost::asio::ip;
        tcp::iostream request_stream;
        if (timeout>0){
            request_stream.expires_from_now(boost::posix_time::milliseconds(timeout));
        }
        request_stream.connect(hostname, port);
        //request_stream.connect(hostname);
        if(!request_stream){
        	std::clog<<"[DLC ERROR] could NOT connect to server "<<hostname<<":"<<port<<endl;
            return;
        }
        request_stream << "GET " << filename << " HTTP/1.0\r\n";
        request_stream << "Host: " << hostname << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Cache-Control: no-cache\r\n";
        request_stream << "Connection: close\r\n\r\n";
        request_stream.flush();
        std::string line1;
        std::getline(request_stream,line1);
        if (!request_stream)
        {
        	std::clog<<"[DLC ERROR] nothing left on request stream\n";
            return;
        }
        std::stringstream response_stream(line1);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream,status_message);
        if (!response_stream||http_version.substr(0,5)!="HTTP/")
        {
        	std::clog<<"[DLC ERROR] some error!\n";
            return;
        }
        if (status_code!=200)
        {
        	std::clog<<"[DLC ERROR] response status code 200\n";
            return;
        }
        std::string header;
        while (std::getline(request_stream, header) && header != "\r")
            headers.push_back(header);
        out_ << request_stream.rdbuf();
        //return status_code;
    }catch(std::exception &e){
        std::clog << e.what() << std::endl;
        return;
    }
    
    std::clog<<"[DLC INFO]  got the catalog from server["<<hostname<<"] port ["<<port<<"] and saved to ["<<outputFilename<<"]"<<endl;
}

void Dlc::setDlcConfig(DlcConfig newDlcConfig){
}

void Dlc::addDownload(int songId){
}

void Dlc::removeDownload(int songId){
}


