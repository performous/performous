#include "screen_downloads.hh"

#include "audio.hh"
#include "configuration.hh"
#include "joystick.hh"
#include "theme.hh"
#include "i18n.hh"
#include <boost/thread.hpp>
#include <boost/bind.hpp>

namespace {
	const float yoff = 0.18; // Offset from center where to place top row
	const float xoff = 0.45; // Offset from middle where to place first column

	bool countRow(std::string needle, std::string const& haystack, int& count) {
		if (haystack.find(needle) != std::string::npos) ++count;
		if (count > 1) return false;
		return true;
	}
}

ScreenDownloads::ScreenDownloads(std::string const& name, Audio& audio): Screen(name), m_audio(audio) {
	m_selector.reset(new Surface(getThemePath("device_selector.svg")));
	m_mic_icon.reset(new Surface(getThemePath("sing_pbox.svg")));
	m_pdev_icon.reset(new Surface(getThemePath("icon_pdev.svg")));
}

void ScreenDownloads::enter() {
	m_theme.reset(new ThemeDownloads());
	m_selected_column = 0;
	// Detect if there is existing advanced configuration and warn that this will override
	if (!config["audio/devices"].isDefault()) {
		ConfigItem::StringList devconf = config["audio/devices"].sl();
		std::map<std::string, int> countmap;
		bool ok = true;
		for (ConfigItem::StringList::const_iterator it = devconf.begin(); it != devconf.end(); ++it) {
			if (!countRow("blue", *it, countmap["blue"])) { ok = false; break; }
			if (!countRow("red", *it, countmap["red"])) { ok = false; break; }
			if (!countRow("green", *it, countmap["green"])) { ok = false; break; }
			if (!countRow("orange", *it, countmap["orange"])) { ok = false; break; }
			if (!countRow("out=", *it, countmap["out="])) { ok = false; break; }
		}
		if (!ok)
			ScreenManager::getSingletonPtr()->dialog(
				_("It seems you have some manual configurations\nincompatible with this user interface.\nSaving these settings will override\nall existing audio device configuration.\nYour other options changes will be saved too."));
	}
	// TODO: Scrolling would be nicer than just zooming out infinitely
	float s = std::min(xoff / 4 / 1.2, yoff*2 / 10 / 1.1);
	m_mic_icon->dimensions.fixedWidth(s);
	m_pdev_icon->dimensions.fixedWidth(s);
}

void ScreenDownloads::exit() { m_theme.reset(); }

void ScreenDownloads::manageEvent(SDL_Event event) {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	input::NavButton nav(input::getNav(event));
	if (nav != input::NONE) {
		if (nav == input::CANCEL || nav == input::SELECT) sm->activateScreen("Intro");
		else if (nav == input::PAUSE) m_audio.togglePause();
		else if (nav == input::LEFT && m_selected_column > 0) --m_selected_column;
		else if (nav == input::RIGHT && m_selected_column < 4-1) ++m_selected_column;
	} else if (event.type == SDL_KEYDOWN) {
		int key = event.key.keysym.sym;
		SDLMod modifier = event.key.keysym.mod;
	}
}

void ScreenDownloads::draw() {
	m_theme->bg.draw();
/*	if (!m_devs.empty()) {
		// Calculate spacing between columns/rows
		const float xstep = (xoff - 0.5 + xoff) / m_mics.size();
		const float ystep = yoff*2 / m_devs.size();
		// Device text & bg
		m_theme->device_bg.dimensions.stretch(std::abs(xoff*2), m_mic_icon->dimensions.h()*0.9).middle();
		m_selector->dimensions.stretch(m_mic_icon->dimensions.w() * 1.75, m_mic_icon->dimensions.h() * 1.75);
		for (size_t i = 0; i <= m_devs.size(); ++i) {
			const float y = -yoff + i*ystep;
			float alpha = 1.0f;
			// "Grey out" devices that doesn't fit the selection
			if (m_mics[m_selected_column].name == "OUT" && !m_devs[i].out) alpha = 0.5f;
			else if (m_mics[m_selected_column].name != "OUT" && !m_devs[i].in) alpha = 0.5f;
			m_theme->device_bg.dimensions.center(y);
			m_theme->device_bg.draw();
			ColorTrans c(Color(1.0, 1.0, 1.0, alpha));
			m_theme->device.dimensions.middle(-xstep*0.5).center(y);
			m_theme->device.draw(i < m_devs.size() ? m_devs[i].desc() : _("- Unassigned -"));
		}
		// Icons
		for (size_t i = 0; i < m_mics.size(); ++i) {
			Surface& srf = (i < m_mics.size()-1) ? *m_mic_icon : *m_pdev_icon;
			{
				ColorTrans c(m_colorMap[m_mics[i].name]);
				srf.dimensions.middle(-xoff + xstep*0.5 + i*xstep).center(-yoff+m_mics[i].dev*ystep);
				srf.draw();
			}
			// Selection indicator
			if (m_selected_column == i)
				m_selector->dimensions.middle(srf.dimensions.xc()).center(srf.dimensions.yc());
		}
		m_selector->draw(); // Position already set in the loop
	}*/
	// Key help
	m_theme->comment_bg.dimensions.stretch(1.0, 0.025).middle().screenBottom(-0.054);
	m_theme->comment_bg.draw();
	m_theme->comment.dimensions.left(-0.48).screenBottom(-0.067);
	m_theme->comment.draw(_("Use arrow keys to configure. Hit Enter/Start to save and test or Esc/Select to cancel. Ctrl + R to reset defaults"));
	// Additional info
	m_theme->comment_bg.dimensions.middle().screenBottom(-0.01);
	m_theme->comment_bg.draw();
	m_theme->comment.dimensions.left(-0.48).screenBottom(-0.023);
	m_theme->comment.draw(_("For advanced device configuration, use command line parameter --audio (use --audiohelp for details)."));
}

