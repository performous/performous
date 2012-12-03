#pragma once

#include <map>
#include <boost/scoped_ptr.hpp>
#include "screen.hh"
#include "glutil.hh"
#include "libda/portaudio.hpp"

class Audio;
class ThemeAudioDevices;

/// options dialogue
class ScreenAudioDevices: public Screen {
  public:
	/// constructor
	ScreenAudioDevices(std::string const& name, Audio& m_audio);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void manageEvent(input::NavEvent const& event);
	void draw();

  private:
	struct Mic {
		Mic(std::string nm, unsigned dv): name(nm), dev(dv) {}
		std::string name;
		unsigned dev;
	};
	void load(); ///< Check what devices are open
	bool save(bool skip_ui_config = false); ///< Save the config to disk xml and then reload
	bool verify(size_t unassigned_id); ///< Check that all were opened after audio reset

	Audio& m_audio;
	boost::scoped_ptr<ThemeAudioDevices> m_theme;
	unsigned int m_selected_column;
	portaudio::DeviceInfos m_devs;
	std::vector<Mic> m_mics;
	std::map<std::string, Color> m_colorMap;
	boost::scoped_ptr<Surface> m_selector;
	boost::scoped_ptr<Surface> m_mic_icon;
	boost::scoped_ptr<Surface> m_pdev_icon;
};

