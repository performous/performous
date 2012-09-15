#pragma once

#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <vector>

struct Torrent {
	Torrent() {};
	std::string name;
	std::string state;
	float progress;
};

class Downloader : boost::noncopyable {
  public:
	Downloader();
	~Downloader();
	void pause(bool state);
	void addTorrent(std::string url);
	std::vector<Torrent> getTorrents() const;
  private:
	class Impl;
	boost::scoped_ptr<Impl> self;
};

