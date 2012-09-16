#include "screen_downloads.hh"

#include "audio.hh"
#include "configuration.hh"
#include "downloader.hh"
#include "joystick.hh"
#include "theme.hh"
#include "i18n.hh"
#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>

ScreenDownloads::ScreenDownloads(std::string const& name, Audio& audio, Downloader& downloader): Screen(name), m_audio(audio), m_downloader(downloader) {
}

void ScreenDownloads::enter() {
	m_theme.reset(new ThemeDownloads());
	m_selectedTorrent = 0;
}

void ScreenDownloads::exit() { m_theme.reset(); }

void ScreenDownloads::manageEvent(SDL_Event event) {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	input::NavButton nav(input::getNav(event));
	if (nav != input::NONE) {
		if (nav == input::CANCEL || nav == input::SELECT) sm->activateScreen("Intro");
		else if (nav == input::LEFT && m_selectedTorrent > 0) --m_selectedTorrent;
		else if (nav == input::RIGHT) ++m_selectedTorrent;
		else if (nav == input::PAUSE) m_audio.togglePause();
	} else if (event.type == SDL_KEYDOWN) {
		int key = event.key.keysym.sym;
		SDLMod modifier = event.key.keysym.mod;
		if (key == SDLK_SPACE) {
			std::vector<Torrent> torrents = m_downloader.getTorrents();
			if(m_selectedTorrent < torrents.size()) {
				m_downloader.pauseResume(torrents[m_selectedTorrent].sha1);
			}
		} else if(key == SDLK_BACKSPACE) {
			std::vector<Torrent> torrents = m_downloader.getTorrents();
			if(m_selectedTorrent < torrents.size()) {
				m_downloader.removeTorrent(torrents[m_selectedTorrent].sha1);
			}
		}
	}
}

namespace {
	std::string addSuffix(boost::int64_t val, std::string suffix = "") {
		std::string unit = "b";
		std::vector<std::string> modifier = boost::assign::list_of("")("k")("M")("H")("T")("P");
		std::string chosenModifier = modifier.back();
		for(std::vector<std::string>::const_iterator it = modifier.begin() ; it != modifier.end() ; ++it) {
			if(val < 1024) {
				chosenModifier = *it;
				break;
			} else {
				val /= 1024;
			}
		}
		std::ostringstream ret;
		ret << boost::lexical_cast<std::string>(val) << " " << chosenModifier << unit << suffix;
		return ret.str();
	}
}

void ScreenDownloads::draw() {
	m_theme->bg.draw();
	std::vector<Torrent> torrents = m_downloader.getTorrents();
	// Torrent information
	std::ostringstream info;
	if(torrents.size() > 0) {
		if(m_selectedTorrent >= torrents.size()) {
			m_selectedTorrent = torrents.size() - 1;
		}

		Torrent& torrent = torrents[m_selectedTorrent];
		info
			<< "Torrent " << (m_selectedTorrent+1) << "/" << torrents.size() << ": "
			<< torrent.name
			<< ", state=" << torrent.state
			<< ", " << boost::lexical_cast<std::string>(int(round(torrent.progress*100))) << "%"
			<< ", size: " << addSuffix(torrent.size)
			<< " (down: " << addSuffix(torrent.downloadRate,"/s") << ", up: " << addSuffix(torrent.uploadRate,"/s") << ")";
	} else if(!Downloader::enabled()) {
		info << _("Torrent support disabled");
	} else {
		info << _("No torrent available");
	}
	m_theme->comment_bg.dimensions.stretch(1.0, 0.025).middle().screenBottom(-0.054);
	m_theme->comment_bg.draw();
	m_theme->comment.dimensions.left(-0.48).screenBottom(-0.067);
	m_theme->comment.draw(info.str());
	// Additional info
	std::ostringstream message;
	message << boost::format(_("Use left/right keys to scroll across the %1% torrent(s) (down: %2%, up: %3%)"))
		% torrents.size() % addSuffix(m_downloader.getDownloadRate(),"/s") % addSuffix(m_downloader.getUploadRate(),"/s");
	m_theme->comment_bg.dimensions.middle().screenBottom(-0.01);
	m_theme->comment_bg.draw();
	m_theme->comment.dimensions.left(-0.48).screenBottom(-0.023);
	m_theme->comment.draw(message.str());
}

