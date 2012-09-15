#pragma once

#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <vector>

struct Torrent {
	Torrent() {};
	std::string name;
	std::string state;
	std::string sha1;
	float progress;
};

class Downloader : boost::noncopyable {
  public:
	Downloader();
	~Downloader();
	void pause(bool state);
	void pauseResume(std::string sha1);
	void addTorrent(std::string url);
	void removeTorrent(std::string sha1);
	std::vector<Torrent> getTorrents() const;
  private:
	class Impl;
	boost::scoped_ptr<Impl> self;
};

