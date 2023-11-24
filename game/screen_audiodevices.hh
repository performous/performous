#pragma once

#include "screen.hh"
#include "graphic/glutil.hh"
#include "libda/portaudio.hpp"
#include "theme/theme.hh"
#include "event_manager.hh"

#include <map>

class Audio;
class ThemeAudioDevices;

/// options dialogue
class ScreenAudioDevices: public Screen {
  public:
	/// constructor
	ScreenAudioDevices(Game &game, std::string const& name, Audio& m_audio);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void manageEvent(input::NavEvent const& event);
	void draw();

  private:
	struct Channel {
		Channel(std::string const& name): name(name), pos(-1) {}
		std::string name;
		int pos;
	};
	void load(); ///< Check what devices are open
	bool save(bool skip_ui_config = false); ///< Save the config to disk xml and then reload
	bool verify(); ///< Check that all were opened after audio reset
	void onEnter(EventParameter const&);

	Audio& m_audio;
	std::shared_ptr<ThemeAudioDevices> m_theme;
	unsigned int m_selected_column = 0;
	portaudio::DeviceInfos m_devs;
	std::vector<Channel> m_channels;
	std::unique_ptr<Texture> m_selector;
	std::unique_ptr<Texture> m_mic_icon;
	std::unique_ptr<Texture> m_pdev_icon;
};

