#include "controllers.hh"

#include <cmath>  // For std::abs
#include <memory>
#include <sstream>
#include <vector>

namespace input {
	struct Joysticks: public Hardware {
		Joysticks() {
			for (int id = 0; id < SDL_NumJoysticks(); ++id) {
				JoyPtr joy(SDL_JoystickOpen(id), SDL_JoystickClose);
				SpdLogger::info(LogSystem::CONTROLLERS,
					"Joystick {} opened: {} ({} buttons, {} axes, {} hats, {} balls.)",
					id,
					getName(id),
					SDL_JoystickNumButtons(joy.get()),
					SDL_JoystickNumAxes(joy.get()),
					SDL_JoystickNumHats(joy.get()),
					SDL_JoystickNumBalls(joy.get())
				);
				m_joysticks.push_back(joy);
			}
		}
		std::string getName(int device) const override {
			return SDL_JoystickNameForIndex(device);
		}
		bool process(Event& event, SDL_Event const& sdlEv) override {
			if (sdlEv.type == SDL_JOYBUTTONDOWN || sdlEv.type == SDL_JOYBUTTONUP) {
				event.hw = sdlEv.jbutton.button;
				event.value = (sdlEv.type == SDL_JOYBUTTONDOWN ? 1.0 : 0.0);
			}
			else if (sdlEv.type == SDL_JOYAXISMOTION) {
				event.hw = hwIsAxis.min + sdlEv.jaxis.axis;
				event.value = (sdlEv.jaxis.value + 0.5) / 32767.5;  // Convert to -1.0 .. 1.0 range
				if (std::abs(event.value) < 0.001) event.value = 0.0;  // Some dead zone around zero
			}
			else if (sdlEv.type == SDL_JOYHATMOTION) {
				event.hw = hwIsHat.min + sdlEv.jhat.hat;
				event.value = sdlEv.jhat.value;
			}
			else return false;
			try {
			event.source = SourceId(SourceType::JOYSTICK, static_cast<unsigned>(sdlEv.jbutton.which));  // All j* structures have .which at the same position as jbutton
			} catch (std::exception const&) {
				SpdLogger::error(LogSystem::CONTROLLERS, "Invalid SDL_JoystickID: {}", std::to_string(sdlEv.jbutton.which));
			}
			return true;
		}
		typedef std::shared_ptr<SDL_Joystick> JoyPtr;
		std::vector<JoyPtr> m_joysticks;

	};

	Hardware::ptr constructJoysticks() { return Hardware::ptr(new Joysticks()); }
}


#if 0

// Forcing menu

void init() {
	std::map<unsigned int, Instrument> forced_type;
	ConfigItem::StringList instruments = config["game/instruments"].sl();

	// Populate controller forcing config items
	ConfigItem& ci0 = config["game/instrument0"];
	ConfigItem& ci1 = config["game/instrument1"];
	ConfigItem& ci2 = config["game/instrument2"];
	ConfigItem& ci3 = config["game/instrument3"];
	unsigned short i = 0;
	for (ControllerDefs::const_iterator it = m_controllerDefs.begin(); it != m_controllerDefs.end(); ++it, ++i) {
		// Add the enum
		std::string title = it->second.description;
		ci0.addEnum(title);
		ci1.addEnum(title);
		ci2.addEnum(title);
		ci3.addEnum(title);
		// Check for active items
		if (i == ci0.ui()-1) instruments.push_back("0:" + it->first);
		if (i == ci1.ui()-1) instruments.push_back("1:" + it->first);
		if (i == ci2.ui()-1) instruments.push_back("2:" + it->first);
		if (i == ci3.ui()-1) instruments.push_back("3:" + it->first);
	}

	// Check all forced instruments
	for (ConfigItem::StringList::const_iterator it = instruments.begin(); it != instruments.end(); ++it) {
		std::istringstream iss(*it);
		unsigned sdl_id;
		char ch;
		std::string type;
		if (!(iss >> sdl_id >> ch >> type) || ch != ':') {
			SpdLogger::error(LogSystem::CONTROLLERS, "Invalid syntax in: {}, should be SDL_ID:CONTROLLER_TYPE", *it);
			continue;
		} else {
			bool found = false;
			for (Instruments::const_iterator it2 = g_instruments.begin(); it2 != g_instruments.end(); ++it2) {
				if (type == it2->first) {
					forced_type.insert(std::pair<unsigned int,Instrument>(sdl_id, Instrument(it2->second)));
					found = true;
					break;
				}
			}
			if (!found) SpdLogger::error(LogSystem::CONTROLLERS, "Unknown controller type: {}.", type);
		}
	}

#endif

