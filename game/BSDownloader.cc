///Implimentation of httpDownloder class
#include <iostream>
#include <fstream>
#include <boost/asio.hpp>

#include "BSDownloader.hh"

using namespace std;

BSDownloader::BSDownloader(){//std::clog<<"[HEYA] inside BSDownloader constructor."<<endl;
}
BSDownloader::~BSDownloader(){//std::clog<<"[BOOM] inside BSDownloader destructor."<<endl;
}
		

void BSDownloader::pause(bool state){
}
void BSDownloader::pauseResume(std::string sha1){}
void BSDownloader::addDownload(std::string url){
	std::clog<<"[:)] inside BSDownloader addDownload."<<endl;
	downloadFile(url);
}
void BSDownloader::removeDownload(std::string sha1){}
std::vector<DLCContent> BSDownloader::getDownloads() const{
	std::vector<DLCContent> vc;
	//DLCContent c;
	return vc;
}
int BSDownloader::getUploadRate() const{return 0;}
int BSDownloader::getDownloadRate() const{return 0;}


void downloadFile(string filename){
	//TODO save file to tmp dir or cache dir
	downloadFile(hostname, port, filename, timeout, filename);
}

void BSDownloader::downloadFile(string hostname, string port, string filename, unsigned int timeout, string outputFilename){

	using boost::asio::ip::tcp;
	
    std::ofstream outFile(outputFilename);
	std::vector<std::string> headers;

	try{
        
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
    
    std::clog<<"[DLC INFO]  got the file["<<filename<<" from server["<<hostname<<"] port ["<<port<<"] and saved to ["<<outputFilename<<"]"<<endl;
}







////////////////////////////////////////////////////////////////
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\TRASH\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////////////////////////////////////

// below commented code uses cpp-netlib to download catalog.
//this function can replcare getCatalog() function
//#define BOOST_THREAD_USE_LIB
/*
namespace {
std::string get_filename(const uri::uri &url) {
    std::string path = uri::path(url);
    std::size_t index = path.find_last_of('/');
    std::string filename = path.substr(index + 1);
    return filename.empty()? "index.html" : filename;
}
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
