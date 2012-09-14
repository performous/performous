#include "downloader.hh"

#include "fs.hh"
#include "config.hh"
#include "configuration.hh"
#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/magnet_uri.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/file.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/storage.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/create_torrent.hpp"
#include <iostream>
#include <fstream>
#include <iterator>
#include <iomanip>

using namespace boost::filesystem;
using namespace libtorrent;

struct Downloader::Impl {
	session s;
	Impl() try {
		session_settings settings;
		settings.user_agent = PACKAGE "/" VERSION " " + settings.user_agent;
		s.set_settings(settings);
		pause(true);
		
		add_torrent_params p;
		p.save_path = (getDataDir() / "songs" / "dlc").string();
		error_code ec;
		add_torrent_params params;
		params.url = "magnet:?xt=urn:btih:7ea1b59cce1737437a66e29a2843b5ce3a0c8cd9";
		s.add_torrent(params, ec);
		pause(false);
	} catch (std::exception& e) {
		std::clog << "downloader/error: " << e.what() << std::endl;
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
			}
			s.start_dht();
			s.start_lsd();
			s.start_upnp();
			s.start_natpmp();
			s.resume();
		}
	}
	void initRepos() {
		/*StringList sl const& = config["dlc/repositories"];
		for (size_t i = 0; i < sl.size(); ++i) {
			//stream.async_open()
		}*/
	}
};

Downloader::Downloader(): self(new Impl) {}
Downloader::~Downloader() {}

void Downloader::pause(bool state) { self->pause(state); }

void Downloader::poll() {
	std::vector<torrent_handle> torrents = self->s.get_torrents();
	torrent_status stat = torrents[0].status();
	//std::cerr << torrents[0].save_path() << " " << stat.state << " " << stat.total_download << " " << stat.error << std::endl;
}
