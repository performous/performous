///This file contains implementation of dlc class
#include "dlc.hh"
#include "fs.hh"

#include <string>
#include <fstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <fstream>
#include <vector>
#include <boost/asio.hpp>
#include <json/json.h>

//#include <json.h>

//Dlc::Dlc(DlcConfig dlcConfig){ }
Dlc::Dlc(){}
Dlc::~Dlc(){}

std::string hostname = "localhost";
std::string filename = "/catalog.json";
std::string catalogFilename = "catalog.json";

//TODO save to config dir. also edit getCatalogFilename()
std::string outputFilename = "catalog.json";
std::string port = "80";
unsigned int timeout = 8000;

	
/// This function trys to get the song catalog from server using boost asio 	
void Dlc::getCatalog(){
	
    std::ofstream outFile(outputFilename);
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
        outFile << request_stream.rdbuf();
        outFile.close();
        //return status_code;
    }catch(std::exception &e){
        std::clog << e.what() << std::endl;
        return;
    }
    
    std::clog<<"[DLC INFO]  got the catalog from server["<<hostname<<"] port ["<<port<<"] and saved to ["<<outputFilename<<"]"<<endl;
}

void printSongInfo(Json::Value song){
	std::clog<<"\n-----------printing a song-------------\n";
	std::clog<<"Name="<<song["name"];
	std::clog<<"Artist="<<song["artist"];
}

void Dlc::parseCatalog(){

	std::ifstream catalogFile(catalogFilename);
	
	Json::Value root;   // will contains the root value after parsing.
	Json::Reader reader;
	bool parsingSuccessful = reader.parse( catalogFile, root );
	if ( !parsingSuccessful ){
		// report to the user the failure and their locations in the document.
		std::clog  << "Failed to parse configuration\n"
		           << reader.getFormattedErrorMessages();
		return;
	}
	
	//parsing songs
	const Json::Value songs = root["songs"];
	for ( int index = 0; index < songs.size(); ++index ){  // Iterates over the sequence elements.
	   printSongInfo(songs[index] );
	}
}


void Dlc::removeDownload(int songId){
}


void Dlc::setDlcConfig(DlcConfig newDlcConfig){
}

void Dlc::addDownload(int songId){

//magnetlink 192.2 kB
// magnet:?xt=urn:btih:cd74d5be7a53cb956a77679219893cd514aeb7ee&dn=Maya Banks - Stay With Me - Epub+pirateflix&tr=udp%3A%2F%2Ftracker.openbittorrent.com%3A80&tr=udp%3A%2F%2Ftracker.publicbt.com%3A80&tr=udp%3A%2F%2Ftracker.istole.it%3A6969&tr=udp%3A%2F%2Ftracker.ccc.de%3A80
	
//magnet 2  3.41 MB
// magnet:?xt=urn:btih:8e5999b45e4b5c9506cf70d39b14cf571edad7d3&dn=The Wall Street Journal Europe - July 30 2013+pirateflix&tr=udp%3A%2F%2Ftracker.openbittorrent.com%3A80&tr=udp%3A%2F%2Ftracker.publicbt.com%3A80&tr=udp%3A%2F%2Ftracker.istole.it%3A6969&tr=udp%3A%2F%2Ftracker.ccc.de%3A80

}


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
#include <boost/network/protocol/http/client.hpp>
#include <boost/network/uri.hpp>

void Dlc::getTheCatalog() {

	namespace http = boost::network::http;
	namespace uri = boost::network::uri;
	std::string url = "localhost/catalog.xml";
    
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

