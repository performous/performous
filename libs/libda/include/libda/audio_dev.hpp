#ifndef AUDIO_DEV_HPP_INCLUDED
#define AUDIO_DEV_HPP_INCLUDED

#include <libda/audio.hpp>
#include <plugin++/plugin.hpp>

namespace da {
	class record::dev {
	  public:
		virtual ~dev() {}
	};
	class playback::dev {
	  public:
		virtual ~dev() {}
	};
	typedef plugin::registry<record::dev, settings&, devinfo> record_plugin;
	typedef plugin::registry<playback::dev, settings&, devinfo> playback_plugin;
}

#endif
