#include "screen_audiodevices.hh"

#include "audio.hh"
#include "configuration.hh"
#include "controllers.hh"
#include "microphones.hh"
#include "platform.hh"
#include "theme.hh"
#include "i18n.hh"
#include "game.hh"
#include "graphic/color_trans.hh"

namespace {
	static const int unassigned_id = -1;  // mic.dev value for unassigned
	static const float yoff = 0.18f; // Offset from center where to place top row
	static const float xoff = 0.45f; // Offset from middle where to place first column

	std::vector<std::string> getMicrophoneColorNames(std::string const& prepend) {
		auto const config = getMicrophoneConfig();
		auto result = std::vector<std::string>{ prepend };

		std::for_each(config.begin(), config.end(), [&result](auto const& config) { result.emplace_back(config.colorname); });

		return result;
	}

	bool countRow(std::string needle, std::string const& haystack, int& count) {
		if (haystack.find(needle) != std::string::npos)
			++count;
		if (count > 1)
			return false;
		return true;
	}

	bool count(ConfigItem::StringList const& devconf, std::map<std::string, int>& countmap) {
		auto const colorNames = getMicrophoneColorNames("out=");

		for (auto& deviceConfig : devconf) {
			for (auto const& colorName : colorNames) {
				if (!countRow(colorName, deviceConfig, countmap[colorName])) {
					return false;
				}
			}
		}

		return true;
	}
}

int getBackend() {
	static std::string selectedBackend = Audio::backendConfig().getValue();
	return PaHostApiNameToHostApiTypeId(selectedBackend);
}


ScreenAudioDevices::ScreenAudioDevices(Game &game, std::string const& name, Audio& audio):
  Screen(game, name), m_audio(audio) {
	m_selector = std::make_unique<Texture>(findFile("device_selector.svg"));
	m_mic_icon = std::make_unique<Texture>(findFile("sing_pbox.svg"));
	m_pdev_icon = std::make_unique<Texture>(findFile("icon_pdev.svg"));
}

void ScreenAudioDevices::enter() {
	int bend = getBackend();
	std::clog << "audio-devices/debug: Entering audio Devices... backend has been detected as: " << bend << std::endl;
	m_theme = std::make_unique<ThemeAudioDevices>();
	PaHostApiTypeId backend = PaHostApiTypeId(bend);
	portaudio::AudioDevices ads(backend);
	m_devs = ads.devices;
	// FIXME: Something more elegant, like a warning box
	if (m_devs.empty()) 
		throw std::runtime_error("No audio devices found!");
	m_selected_column = 0;
	// Detect if there is existing advanced configuration and warn that this will override
	if (!config["audio/devices"].isDefault()) {
		auto const devconf = config["audio/devices"].sl();
		auto countmap = std::map<std::string, int>{};
		auto const ok = count(devconf, countmap);
		if (!ok)
			getGame().dialog(
				_("It seems you have some manual configurations\nincompatible with this user interface.\nSaving these settings will override\nall existing audio device configuration.\nYour other options changes will be saved too."));
	}
	// Populate the mics vector and check open devices
	load();
	// TODO: Scrolling would be nicer than just zooming out infinitely
	float s = std::min(xoff / static_cast<float>(m_channels.size()) / 1.2f, yoff*2 / static_cast<float>(m_devs.size()) / 1.1f);
	m_mic_icon->dimensions.fixedWidth(s);
	m_pdev_icon->dimensions.fixedWidth(s);
}

void ScreenAudioDevices::exit() {
	m_theme.reset();
}

void ScreenAudioDevices::manageEvent(input::NavEvent const& event) {
	input::NavButton nav = event.button;
	int& chpos = m_channels[m_selected_column].pos;
	const int posN = static_cast<int>(m_devs.size() + 1);

	if (nav == input::NavButton::CANCEL) getGame().activateScreen("Intro");
	else if (nav == input::NavButton::PAUSE) m_audio.togglePause();
	else if (m_devs.empty()) return; // The rest work if there are any devices
	else if (nav == input::NavButton::START) { if (save()) getGame().activateScreen("Intro"); }
	else if (nav == input::NavButton::LEFT && m_selected_column > 0) --m_selected_column;
	else if (nav == input::NavButton::RIGHT && m_selected_column < m_channels.size()-1) ++m_selected_column;
	else if (nav == input::NavButton::UP) chpos = static_cast<int>((chpos + posN) % posN - 1);
	else if (nav == input::NavButton::DOWN) chpos = static_cast<int>((chpos + posN + 2) % posN - 1);
}

void ScreenAudioDevices::manageEvent(SDL_Event event) {
	if (event.type == SDL_KEYDOWN) {
		int key = event.key.keysym.scancode;
		std::uint16_t modifier = event.key.keysym.mod;
		if (m_devs.empty()) return; // The rest work if there are any config options
		// Reset to defaults
		else if (key == SDL_SCANCODE_R && modifier & Platform::shortcutModifier()) {
			config["audio/devices"].reset(modifier & KMOD_ALT);
			save(true); // Save to disk, reload audio & reload UI to keep stuff consistent
		}
	}
}

void ScreenAudioDevices::draw() {
	auto& window = getGame().getWindow();

	m_theme->bg.draw(window);
	if (m_devs.empty()) return;
	// Calculate spacing between columns/rows
	const float xstep = (xoff - 0.5f + xoff) / static_cast<float>(m_channels.size());
	const float ystep = yoff*2 / static_cast<float>(m_devs.size());
	// Device text & bg
	m_theme->device_bg.dimensions.stretch(std::abs(xoff*2.15f), m_mic_icon->dimensions.h()*0.9f).middle();
	m_selector->dimensions.stretch(m_mic_icon->dimensions.w() * 1.75f, m_mic_icon->dimensions.h() * 1.75f);
	for (size_t i = 0; i < m_devs.size(); ++i) {
		const float y = -yoff + static_cast<float>(i)*ystep;
		float alpha = 1.0f;

		const bool isDevice = (i < m_devs.size());
		if (isDevice) {
			// "Grey out" devices that doesn't fit the selection
			if (m_channels[m_selected_column].name == "OUT" && !m_devs[i].out)
				alpha = 0.5f;
			else if (m_channels[m_selected_column].name != "OUT" && !m_devs[i].in)
				alpha = 0.5f;
		}
		m_theme->device_bg.dimensions.center(y);
		m_theme->device_bg.draw(window);
		ColorTrans c(window, Color::alpha(alpha));
		m_theme->device.dimensions.middle(-xstep*0.5f).center(y);
		m_theme->device.draw(window, isDevice ? m_devs[i].desc() : _("- Unassigned -"));
	}
	// Icons
	for (size_t i = 0; i < m_channels.size(); ++i) {
		auto& srf = m_channels[i].name != "OUT" ? *m_mic_icon : *m_pdev_icon;
		{
			ColorTrans c(window, getMicrophoneColor(m_channels[i].name));
			int pos = m_channels[i].pos;
			if (pos == unassigned_id) 
				pos = static_cast<int>(m_devs.size());  // Transform -1 to the bottom of the list
			srf.dimensions.middle(-xoff + xstep*0.5f + static_cast<float>(i)*xstep).center(-yoff+static_cast<float>(pos)*ystep);
			srf.draw(window);
		}
		// Selection indicator
		if (m_selected_column == i)
			m_selector->dimensions.middle(srf.dimensions.xc()).center(srf.dimensions.yc());
	}
	m_selector->draw(window); // Position already set in the loop
	// Key help
	m_theme->comment_bg.dimensions.stretch(1.0f, 0.025f).middle().screenBottom(-0.054f);
	m_theme->comment_bg.draw(window);
	m_theme->comment.dimensions.left(-0.48f).screenBottom(-0.067f);
	m_theme->comment.draw(window, _("Use arrow keys to configure. Hit Enter/Start to save and test or Esc/Select to cancel. Ctrl + R to reset defaults"));
	// Additional info
	m_theme->comment_bg.dimensions.middle().screenBottom(-0.01f);
	m_theme->comment_bg.draw(window);
	m_theme->comment.dimensions.left(-0.48f).screenBottom(-0.023f);
	m_theme->comment.draw(window, _("For advanced device configuration, use command line parameter --audio (use --audiohelp for details)."));
}

void ScreenAudioDevices::load() {
	assignChannels();

	// Get the currently assigned devices for each channel (FIXME: this is a really stupid algorithm)
	for (auto const& device: m_audio.devices()) {
		for (auto& channel: m_channels) {
			if (!device.isChannel(channel.name))
				continue;
			for (int i = 0; i < static_cast<int>(m_devs.size()); ++i) {
				if (m_devs[static_cast<size_t>(i)].index == device.dev) 
					channel.pos = i;
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
				if (c.pos == unassigned_id || m_devs[static_cast<size_t>(c.pos)].idx != d.idx) 
					continue;
				if (c.name == "OUT")
					pdev = "out=2"; // Pdev, only stereo supported
				else { // Mic
					if (!mics.empty())
						mics += ","; // Add separator if needed
					mics += c.name; // Append mic color
				}
			}
			if (mics.empty() && pdev.empty())
				continue; // Continue looping if device is not used
			std::string dev = "dev=\"" + d.flex + "\""; // Use flexible name for more robustness
			// Use half duplex I/O even if the same device is used for capture and playback (works better)
			if (!mics.empty())
				devconf.push_back(dev + " mics=" + mics);
			if (!pdev.empty())
				devconf.push_back(dev + " " + pdev);
		}
		config["audio/devices"].sl() = devconf;
	}
	writeConfig(getGame(), false); // Save the new config
	m_audio.restart(); // Reload audio to take the new settings into use
	m_audio.playMusic(getGame(), findFile("menu.ogg"), true); // Start music again
	// Check that all went well
	bool ret = verify();
	if (!ret)
		getGame().dialog(_("Some devices failed to open!"));
	// Load the new config back for UI
	load();
	return ret;
}

bool ScreenAudioDevices::verify() {
	for (auto const& channel: m_channels) {
		if (channel.pos == unassigned_id) 
			continue;  // No checking needed of unassigned channels
		// Find the device
		for (auto const& device: m_audio.devices()) 
			if (device.isChannel(channel.name))
				goto found;
		return false;
		found:;
	}
	return true;
}

void ScreenAudioDevices::assignChannels() {
	auto const colorNames = getMicrophoneColorNames("OUT");

	m_channels.assign(std::begin(colorNames), std::end(colorNames));
}
