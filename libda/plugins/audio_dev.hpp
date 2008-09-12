#ifndef AUDIO_DEV_HPP_INCLUDED
#define AUDIO_DEV_HPP_INCLUDED

#include <libda/audio.hpp>
#include "plugin.hpp"

namespace da {
	struct record::dev {
		virtual ~dev() {}
	};
	struct playback::dev {
		virtual ~dev() {}
	};
	typedef plugin::registry<record::dev, settings&, devinfo> record_plugin;
	typedef plugin::registry<playback::dev, settings&, devinfo> playback_plugin;
}

#endif
