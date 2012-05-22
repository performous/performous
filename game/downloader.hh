#pragma once

#include <boost/scoped_ptr.hpp>

class Downloader {
public:
	Downloader();
	~Downloader();
	void poll();
	void pause(bool state);
private:
	struct Impl;
	boost::scoped_ptr<Impl> self;
};

