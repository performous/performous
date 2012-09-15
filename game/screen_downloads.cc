#include "screen_downloads.hh"

#include "audio.hh"
#include "configuration.hh"
#include "downloader.hh"
#include "joystick.hh"
#include "theme.hh"
#include "i18n.hh"
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
			<< ", " << boost::lexical_cast<std::string>(int(round(torrent.progress*100))) << "%";
	} else {
		info << _("No torrent available");
	}
	m_theme->comment_bg.dimensions.stretch(1.0, 0.025).middle().screenBottom(-0.054);
	m_theme->comment_bg.draw();
	m_theme->comment.dimensions.left(-0.48).screenBottom(-0.067);
	m_theme->comment.draw(info.str());
	// Additional info
	std::ostringstream message;
	message << boost::format(_("Use left/right keys to scoll accross the %1% torrent(s)")) % torrents.size();
	m_theme->comment_bg.dimensions.middle().screenBottom(-0.01);
	m_theme->comment_bg.draw();
	m_theme->comment.dimensions.left(-0.48).screenBottom(-0.023);
	m_theme->comment.draw(message.str());
}

