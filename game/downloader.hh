#pragma once

#include <boost/cstdint.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <vector>
#include <string>

struct Torrent {
	Torrent() {};
	std::string name;
	std::string state;
	std::string sha1;
	float progress;
	int uploadRate;
	int downloadRate;
	boost::int64_t size;
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
	int getUploadRate() const;
	int getDownloadRate() const;
	static bool enabled() {
		#ifdef USE_TORRENT
		return true;
		#else
		return false;
		#endif
	}
  private:
	class Impl;
	boost::scoped_ptr<Impl> self;
};

