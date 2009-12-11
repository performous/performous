#ifndef AUDIO_DEV_HPP_INCLUDED
#define AUDIO_DEV_HPP_INCLUDED

/**
@file audio_dev.hpp Audio plugin interface. Not very stable yet.

This requires linking with libda and the ABI is not stable yet. The API might
also change.
**/

#ifdef BUILDING_DA
#define PLUGIN_MASTER
#endif

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
	namespace {
		record_plugin::init record_init;
		playback_plugin::init playback_init;
	}
}

#endif
