/*
This file contains implementation of dlc class
*/

#include "dlc.hh"

/*
#include <boost/network/protocol/http/client.hpp>
#include <boost/network/uri.hpp>
*/
#include <string>
#include <fstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <boost/asio.hpp>


//#define BOOST_THREAD_USE_LIB
//namespace http = boost::network::http;
//namespace uri = boost::network::uri;

/*
namespace {
std::string get_filename(const uri::uri &url) {
    std::string path = uri::path(url);
    std::size_t index = path.find_last_of('/');
    std::string filename = path.substr(index + 1);
    return filename.empty()? "index.html" : filename;
}

} // namespace

*/

Dlc::Dlc(DlcConfig dlcConfig){}
Dlc::Dlc(){}
Dlc::~Dlc(){}

std::string url = "localhost/catalog.xml";
std::string hostname = "localhost";
std::string filename = "/catalog.txt";

/*
void Dlc::getTheCatalog() {

	

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

void Dlc::getCatalog(){
	
// sync_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


using boost::asio::ip::tcp;

  try
  {

    boost::asio::io_service io_service;

    // Get a list of endpoints corresponding to the server name.
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(hostname, "http");
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    // Try each endpoint until we successfully establish a connection.
    tcp::socket socket(io_service);
    boost::asio::connect(socket, endpoint_iterator);

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    boost::asio::streambuf request;
    std::ostream request_stream(&request);
    request_stream << "GET " << filename << " HTTP/1.0\r\n";
    request_stream << "Host: " << hostname << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    // Send the request.
    boost::asio::write(socket, request);

    // Read the response status line. The response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    boost::asio::streambuf response;
    boost::asio::read_until(socket, response, "\r\n");

    // Check that response is OK.
    std::istream response_stream(&response);
    std::string http_version;
    response_stream >> http_version;
    unsigned int status_code;
    response_stream >> status_code;
    std::string status_message;
    std::getline(response_stream, status_message);
    if (!response_stream || http_version.substr(0, 5) != "HTTP/")
    {
      std::clog << "Invalid response\n";
      return;
    }
    if (status_code != 200)
    {
      std::clog << "Response returned with status code " << status_code << "\n";
      //return;
    }

    // Read the response headers, which are terminated by a blank line.
    boost::asio::read_until(socket, response, "\r\n\r\n");

    // Process the response headers.
    std::string header;
    while (std::getline(response_stream, header) && header != "\r")
      std::clog << header << "\n";
    std::clog << "\n";

    // Write whatever content we already have to output.
    if (response.size() > 0)
      std::clog << &response;

    // Read until EOF, writing data to output as we go.
    boost::system::error_code error;
    while (boost::asio::read(socket, response,
          boost::asio::transfer_at_least(1), error))
      {
      	std::string filename = "myCatalog.txt";
		std::clog << "Saving to: " << filename << std::endl;
		std::ofstream ofs(filename.c_str());
		//ofs << static_cast<std::string>(response_stream) << std::endl;
      	std::clog << &response;
      	ofs << &response << std::endl;
      }
    if (error != boost::asio::error::eof)
      throw boost::system::system_error(error);
  }
  catch (std::exception& e)
  {
    std::clog << "Exception: " << e.what() << "\n";
  }
  
	std::clog<<"************* got the catalog :) **************"<<endl;
}

void Dlc::setDlcConfig(DlcConfig newDlcConfig){
}

void Dlc::addDownload(int songId){
}

void Dlc::removeDownload(int songId){
}


