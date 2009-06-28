#pragma once

#include <libda/audio.hpp>
#include "pitch.hh"
#include <boost/ptr_container/ptr_vector.hpp>

/// class for recording sound from mics
class Capture {
  private:
	class Device {
	  private:
		std::vector<Analyzer*> m_channels;
		da::settings m_settings;
		da::record m_record;
	  public:
		Device(Capture& c, std::size_t channels, std::size_t rate, std::size_t frames, std::string device):
		  m_channels(channels),
		  m_settings(device),
		  m_record(m_settings.set_callback(boost::ref(*this)).set_channels(channels).set_rate(rate).set_frames(frames).set_debug(std::cerr))
		{
			for(std::size_t ch = 0; ch < channels; ++ch) {
				if (c.m_analyzers.size() < 4) c.m_analyzers.push_back(m_channels[ch] = new Analyzer(m_settings.rate()));
			}
		}
		void operator()(da::pcm_data& areas, da::settings const&) {
			for(std::size_t ch = 0; ch < m_channels.size(); ++ch) {
				if (m_channels[ch]) m_channels[ch]->input(areas.begin(ch), areas.end(ch));
			}
		}
	}; // Device
	boost::ptr_vector<Analyzer> m_analyzers;  // This must come before the devices for correct destruction order
	boost::ptr_vector<Device> m_devices;

  public:
	/// add new microphones
	void addMics(std::size_t channels, std::size_t rate, std::size_t frames, std::string device){
		m_devices.push_back(new Device(*this, channels, rate, frames, device));
	}
	/// get analyzers
	boost::ptr_vector<Analyzer>& analyzers() { return m_analyzers; }
};

