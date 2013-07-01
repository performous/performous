#pragma once
#include <boost/network/protocol/http/server.hpp>
#include <boost/thread/thread.hpp>
#include <string>

using boost::thread;

class WebServer
{
public:
	WebServer();
private:
	boost::thread serverthread;
};

