#include "screen_audiodevices.hh"

#include "audio.hh"
#include "configuration.hh"
#include "controllers.hh"
#include "platform.hh"
#include "theme.hh"
#include "i18n.hh"
#include <boost/thread.hpp>
#include <boost/bind.hpp>

namespace {
	static const int unassigned_id = -1;  // mic.dev value for unassigned
	static const float yoff = 0.18; // Offset from center where to place top row
	static const float xoff = 0.45; // Offset from middle where to place first column

	bool countRow(std::string needle, std::string const& haystack, int& count) {
		if (haystack.find(needle) != std::string::npos) ++count;
		if (count > 1) return false;
		return true;
	}
}

int getBackend() {
	static std::string selectedBackend = Audio::backendConfig().getValue();
	return PaHostApiNameToHostApiTypeId(selectedBackend);
}


ScreenAudioDevices::ScreenAudioDevices(std::string const& name, Audio& audio): Screen(name), m_audio(audio) {
	m_selector.reset(new Surface(findFile("device_selector.svg")));
	m_mic_icon.reset(new Surface(findFile("sing_pbox.svg")));
	m_pdev_icon.reset(new Surface(findFile("icon_pdev.svg")));
}

void ScreenAudioDevices::enter() {
	int bend = getBackend();
	std::clog << "audio-devices/debug: Entering audio Devices... backend has been detected as: " << bend << std::endl;
	m_theme.reset(new ThemeAudioDevices());
	PaHostApiTypeId backend = PaHostApiTypeId(bend);
	portaudio::AudioDevices ads(backend);
	m_devs = ads.devices;
	// FIXME: Something more elegant, like a warning box
	if (m_devs.empty()) throw std::runtime_error("No audio devices found!");
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
			if (!countRow("yellow", *it, countmap["yellow"])) { ok = false; break; }
			if(!countRow("fuchsia", *it, countmap["fuchsia"])) { ok = false; break; }
			if(!countRow("orange", *it, countmap["orange"])) { ok = false; break; }
			if(!countRow("purple", *it, countmap["purple"])) { ok = false; break; }
			if(!countRow("aqua", *it, countmap["aqua"])) { ok = false; break; }
			if(!countRow("white", *it, countmap["white"])) { ok = false; break; }
			if(!countRow("gray", *it, countmap["gray"])) { ok = false; break; }
			if(!countRow("black", *it, countmap["black"])) { ok = false; break; }
			if (!countRow("out=", *it, countmap["out="])) { ok = false; break; }
		}
		if (!ok)
			Game::getSingletonPtr()->dialog(
				_("It seems you have some manual configurations\nincompatible with this user interface.\nSaving these settings will override\nall existing audio device configuration.\nYour other options changes will be saved too."));
	}
	// Populate the mics vector and check open devices
	load();
	// TODO: Scrolling would be nicer than just zooming out infinitely
	float s = std::min(xoff / m_channels.size() / 1.2, yoff*2 / m_devs.size() / 1.1);
	m_mic_icon->dimensions.fixedWidth(s);
	m_pdev_icon->dimensions.fixedWidth(s);
}

void ScreenAudioDevices::exit() { m_theme.reset(); }

void ScreenAudioDevices::manageEvent(input::NavEvent const& event) {
	Game* gm = Game::getSingletonPtr();
	input::NavButton nav = event.button;
	auto& chpos = m_channels[m_selected_column].pos;
	const unsigned posN = m_devs.size() + 1;
	if (nav == input::NAV_CANCEL) gm->activateScreen("Intro");
	else if (nav == input::NAV_PAUSE) m_audio.togglePause();
	else if (m_devs.empty()) return; // The rest work if there are any devices
	else if (nav == input::NAV_START) { if (save()) gm->activateScreen("Intro"); }
	else if (nav == input::NAV_LEFT && m_selected_column > 0) --m_selected_column;
	else if (nav == input::NAV_RIGHT && m_selected_column < m_channels.size()-1) ++m_selected_column;
	else if (nav == input::NAV_UP) chpos = (chpos + posN) % posN - 1;
	else if (nav == input::NAV_DOWN) chpos = (chpos + posN + 2) % posN - 1;
}

void ScreenAudioDevices::manageEvent(SDL_Event event) {
	if (event.type == SDL_KEYDOWN) {
		int key = event.key.keysym.scancode;
		uint16_t modifier = event.key.keysym.mod;
		if (m_devs.empty()) return; // The rest work if there are any config options
		// Reset to defaults
		else if (key == SDL_SCANCODE_R && modifier & Platform::shortcutModifier()) {
			config["audio/devices"].reset(modifier & KMOD_ALT);
			save(true); // Save to disk, reload audio & reload UI to keep stuff consistent
		}
	}
}

void ScreenAudioDevices::draw() {
	m_theme->bg.draw();
	if (m_devs.empty()) return;
	// Calculate spacing between columns/rows
	const float xstep = (xoff - 0.5 + xoff) / m_channels.size();
	const float ystep = yoff*2 / m_devs.size();
	// Device text & bg
	m_theme->device_bg.dimensions.stretch(std::abs(xoff*2.15), m_mic_icon->dimensions.h()*0.9).middle();
	m_selector->dimensions.stretch(m_mic_icon->dimensions.w() * 1.75, m_mic_icon->dimensions.h() * 1.75);
	for (size_t i = 0; i <= m_devs.size(); ++i) {
		const float y = -yoff + i*ystep;
		float alpha = 1.0f;
		// "Grey out" devices that doesn't fit the selection
		if (m_channels[m_selected_column].name == "OUT" && !m_devs[i].out) alpha = 0.5f;
		else if (m_channels[m_selected_column].name != "OUT" && !m_devs[i].in) alpha = 0.5f;
		m_theme->device_bg.dimensions.center(y);
		m_theme->device_bg.draw();
		ColorTrans c(Color::alpha(alpha));
		m_theme->device.dimensions.middle(-xstep*0.5).center(y);
		m_theme->device.draw(i < m_devs.size() ? m_devs[i].desc() : _("- Unassigned -"));
	}
	// Icons
	for (size_t i = 0; i < m_channels.size(); ++i) {
		Surface& srf = (i < m_channels.size()-1) ? *m_mic_icon : *m_pdev_icon;
		{
			ColorTrans c(MicrophoneColor::get(m_channels[i].name));
			int pos = m_channels[i].pos;
			if (pos == unassigned_id) pos = m_devs.size();  // Transform -1 to the bottom of the list
			srf.dimensions.middle(-xoff + xstep*0.5 + i*xstep).center(-yoff+pos*ystep);
			srf.draw();
		}
		// Selection indicator
		if (m_selected_column == i)
			m_selector->dimensions.middle(srf.dimensions.xc()).center(srf.dimensions.yc());
	}
	m_selector->draw(); // Position already set in the loop
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

void ScreenAudioDevices::load() {
	std::string names[] = { "blue", "red", "green", "yellow", "fuchsia", "orange", "purple", "aqua", "white", "gray", "black", "OUT" }; //there were 4 colors here
	m_channels.assign(std::begin(names), std::end(names));
	// Get the currently assigned devices for each channel (FIXME: this is a really stupid algorithm)
	for (auto const& d: m_audio.devices()) {
		for (auto& c: m_channels) {
			if (!d.isChannel(c.name)) continue;
			for (size_t i = 0; i < m_devs.size(); ++i) {
				if (unsigned(m_devs[i].idx) == d.dev) c.pos = i;
			}
		}
	}
}

bool ScreenAudioDevices::save(bool skip_ui_config) {
	if (!skip_ui_config) {
		ConfigItem::StringList devconf;
		// Loop through the devices and if there is mics/pdev assigned, form a config line
		for (auto const& d: m_devs) {  // PortAudio devices
			std::string mics = "", pdev = "";
			for (auto const& c: m_channels) {  // blue, red, ..., OUT
				if (c.pos == unassigned_id || m_devs[c.pos].idx != d.idx) continue;
				if (c.name == "OUT") pdev = "out=2"; // Pdev, only stereo supported
				else { // Mic
					if (!mics.empty()) mics += ","; // Add separator if needed
					mics += c.name; // Append mic color
				}
			}
			if (mics.empty() && pdev.empty()) continue; // Continue looping if device is not used
			std::string dev = "dev=\"" + d.flex + "\""; // Use flexible name for more robustness
			// Use half duplex I/O even if the same device is used for capture and playback (works better)
			if (!mics.empty()) devconf.push_back(dev + " mics=" + mics);
			if (!pdev.empty()) devconf.push_back(dev + " " + pdev);
		}
		config["audio/devices"].sl() = devconf;
	}
	writeConfig(m_audio,false); // Save the new config
	// Give audio a little time to shutdown but then just quit
	boost::thread audiokiller(boost::bind(&Audio::close, boost::ref(m_audio)));
	if (!audiokiller.timed_join(boost::posix_time::milliseconds(2500)))
		Game::getSingletonPtr()->fatalError("Audio hung for some reason.\nPlease restart Performous.");
	m_audio.restart(); // Reload audio to take the new settings into use
	m_audio.playMusic(findFile("menu.ogg"), true); // Start music again
	// Check that all went well
	bool ret = verify();
	if (!ret) Game::getSingletonPtr()->dialog(_("Some devices failed to open!"));
	// Load the new config back for UI
	load();
	return ret;
}

bool ScreenAudioDevices::verify() {
	for (auto const& c: m_channels) {
		if (c.pos == unassigned_id) continue;  // No checking needed of unassigned channels
		// Find the device
		for (auto const& d: m_audio.devices()) if (d.isChannel(c.name)) goto found;
		return false;
		found:;
	}
	return true;
}
