#include "downloader.hh"

#include "fs.hh"
#include "config.hh"
#include "configuration.hh"
#include "xtime.hh"
#include <iostream>
#include <fstream>
#include <iterator>
#include <iomanip>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

using namespace boost::filesystem;

#ifdef USE_TORRENT

#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/magnet_uri.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/file.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/storage.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/torrent_info.hpp"

using namespace libtorrent;

static char const* state_str[] = {"checking (q)", "checking", "dl metadata", "downloading", "finished", "seeding", "allocating", "checking (r)"};

class Downloader::Impl {
  private:
	session s;
	boost::scoped_ptr<boost::thread> m_thread;
	volatile bool m_quit;
	// mutex to protect m_torrents
	mutable boost::mutex m_mutex;
	std::vector<Torrent> m_torrents;

  public:
	int m_uploadRate;
	int m_downloadRate;
	Impl() try {
		m_quit = false;
		session_settings settings;
		settings.user_agent = PACKAGE "/" VERSION " " + settings.user_agent;
		s.set_settings(settings);
		pause(true);
		m_thread.reset(new boost::thread(boost::ref(*this)));
	} catch (std::exception& e) {
		std::clog << "downloader/error: " << e.what() << std::endl;
	}

	~Impl() {
		m_quit = true;
		m_thread->join();
	}

	void operator()() {
		while (!m_quit) {
			// first dequeue alerts
			std::deque<alert*> alerts;
			s.pop_alerts(&alerts);
			for(std::deque<alert*>::const_iterator ita = alerts.begin() ; ita != alerts.end(); ++ita) {
				std::clog << "torrent/error: " << (*ita)->what() << ": " << (*ita)->message() << std::endl;
			}

			// update session information
			session_status ss = s.status();
			m_uploadRate = ss.upload_rate;
			m_downloadRate = ss.download_rate;
			// update torrent information
			std::vector<Torrent> torrents;
			std::vector<torrent_handle> torrent_handles = s.get_torrents();
			for(std::vector<torrent_handle>::const_iterator it = torrent_handles.begin() ; it != torrent_handles.end() ; ++it) {
				const torrent_handle& h = *it;
				torrent_status status = h.status();
				Torrent t;
				t.name = h.name();
				if(status.paused) {
					t.state = "paused";
				} else {
					t.state = state_str[status.state];
				}
				t.progress = status.progress;
				t.uploadRate = status.upload_rate;
				t.downloadRate = status.download_rate;
				t.sha1 = h.info_hash().to_string();
				try {
					torrent_info const& info = h.get_torrent_info();
					t.size = info.total_size();
				} catch(libtorrent_exception &ex) {
					// do nothing, info are just not available
					t.size = 0;
				}
				torrents.push_back(t);

				std::string percent =  boost::lexical_cast<std::string>(int(round(t.progress*100)));
				std::clog << "torrent/debug: " << t.name << " (" << t.state << ":" << percent << "%)" << std::endl;
				/*
				// display files when metadata are here
				try {
					const torrent_info& info =  h.get_torrent_info();
					for(unsigned int i = 0 ; i < info.num_files() ; ++i) {
						std::string filename = info.file_at(i).path;
						std::cout << "  - " << filename << std::endl;
					}
				} catch(libtorrent_exception &ex) {
					// do nothing
				}
				*/
			}
			{
				// update the torrent:
				boost::mutex::scoped_lock l(m_mutex);
				m_torrents = torrents;
			}
			// Sleep a little, much if the cam isn't active
			boost::thread::sleep(now() + 0.2);
		}
	}

	void removeTorrent(std::string sha1) {
		torrent_handle h = s.find_torrent(sha1_hash(sha1));
		s.remove_torrent(h);
	}

	void pauseResume(std::string sha1) {
		torrent_handle h = s.find_torrent(sha1_hash(sha1));
		if(h.status().paused) {
			h.resume();
		} else {
			h.auto_managed(false);
			h.pause();
		}
	}

	void pause(bool state) {
		if (state) {
			s.pause();
			s.stop_natpmp();
			s.stop_upnp();
			s.stop_lsd();
			s.stop_dht();
		} else {
			if (!s.is_listening()) {
				error_code ec;
				s.listen_on(std::make_pair(6881, 6889), ec);
				if(ec) {
					std::clog << "torrent/error: cannot listen for torrents: " << ec.message() << std::endl;
				}
			}
			s.start_dht();
			s.start_lsd();
			s.start_upnp();
			s.start_natpmp();
			s.resume();
		}
	}

	void addTorrent(std::string url) {
		// search for save path
		fs::path savePath = pathMangle(fs::path(config["dlc/save_path"].s()));
		if(*savePath.begin() == "DATADIR") {
			savePath = savePath.string().substr(7);
			savePath = getDataDir() / savePath;
		}
		// fill torrent params
		error_code ec;
		add_torrent_params params;
		params.save_path = savePath.string();
		if (boost::starts_with(url, "hash:")) {
			// we have a hash
			params.info_hash = sha1_hash(url.substr(sizeof("hash:")-1));
		} else if (boost::starts_with(url, "torrent:")) {
			// we have a torrent file
			params.ti = new torrent_info(url.substr(sizeof("torrent:")-1).c_str(), ec);
			if(ec) {
				std::clog << "torrent/error: cannot add torrent file: " << ec.message() << std::endl;
				return;
			}
		} else {
			// we have an url (http, magnet, https, other)
			params.url = url;
		}
		if(config["dlc/autostart"].b()) {
			params.flags |= add_torrent_params::flag_auto_managed;
		} else {
			params.flags &= ~add_torrent_params::flag_auto_managed;
		}
		std::clog << "torrent/info: adding " << url << " (saving in " << savePath << ")" << std::endl;
		s.add_torrent(params, ec);
		if(ec) {
			std::clog << "torrent/error: cannot add torrent: " << ec.message() << std::endl;
		}
	}

	std::vector<Torrent> getTorrents() const {
		std::vector<Torrent> result;
		{
			boost::mutex::scoped_lock l(m_mutex);
			result = m_torrents;
		}
		return result;
	}
};
#else
class Downloader::Impl {
  public:
	Impl() : m_uploadRate(0), m_downloadRate(0) {}
	void pause(bool state) {}
	void pauseResume(std::string sha1) {}
	void addTorrent(std::string url) {}
	void removeTorrent(std::string sha1) {}
	std::vector<Torrent> getTorrents() const {
		std::vector<Torrent> result;
		return result;
	}
  public:
	int m_uploadRate;
	int m_downloadRate;
};
#endif

Downloader::Downloader(): self(new Impl) {
	ConfigItem::StringList urls = config["dlc/torrent_urls"].sl();
	for (ConfigItem::StringList::const_iterator it = urls.begin(), end = urls.end(); it != end; ++it) {
		self->addTorrent(*it);
	}
	pause(false);
}
Downloader::~Downloader() {}

void Downloader::pause(bool state) { self->pause(state); }

void Downloader::pauseResume(std::string sha1) { self->pauseResume(sha1); }

void Downloader::addTorrent(std::string url) { self->addTorrent(url); }

void Downloader::removeTorrent(std::string sha1) { self->removeTorrent(sha1); }

std::vector<Torrent> Downloader::getTorrents() const { return self->getTorrents(); };

int Downloader::getUploadRate() const { return self->m_uploadRate; };

int Downloader::getDownloadRate() const { return self->m_downloadRate; };
