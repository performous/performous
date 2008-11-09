#ifndef __RECORD_H_
#define __RECORD_H_

#include <audio.hpp>
#include "pitch.hh"
#include <boost/ptr_container/ptr_vector.hpp>
#include <iostream>

class Capture {
	static const std::size_t DEFAULT_RATE = 48000;
	volatile bool m_running;
	std::size_t channel[4];
	da::settings *settings[4];
	da::record *records[4];
	boost::ptr_vector<Analyzer> m_analyzers;
  public:
	Capture(std::string const& device1ch = "", std::string const& device2ch = "", std::string const& device3ch = "", std::string const& device4ch = "", std::size_t rate = DEFAULT_RATE):
	  m_running(false)
	{
		addPlayer(0,device1ch,rate);
		if(device2ch!=""){addPlayer(1,device2ch,rate);}
		if(device3ch!=""){addPlayer(2,device3ch,rate);}
		if(device4ch!=""){addPlayer(3,device4ch,rate);}
		m_running = true;
	}
	inline void addPlayer(std::size_t player, std::string const& device_and_ch, std::size_t rate){
		// FIXME two exception for the cdev="" case:
		const std::string full_device = device_and_ch==""?"":device_and_ch.substr(0,device_and_ch.find('%'));
		channel[player] = device_and_ch==""?0:(boost::lexical_cast<int>(device_and_ch.substr(device_and_ch.find('%')+1)));
		settings[player] = new da::settings(full_device);
		settings[player]->set_callback(boost::ref(*this));
		settings[player]->set_channels(2);
		settings[player]->set_rate(rate);
		settings[player]->set_debug(std::cerr);
		records[player] = new da::record(*settings[player]);
		m_analyzers.push_back(new Analyzer(settings[player]->rate()));
		std::cout << ">>> Player " << (player+1) << ": device=" << settings[player]->device() << " subdev=" << settings[player]->subdev() << " channel=" << channel[player] << std::endl << std::endl;
	}
	void operator()(da::pcm_data& areas, da::settings const& rs) {
		if (!m_running) return;
		//std::cout << ">>> sound; device=" << rs.device() << " subdev=" << rs.subdev() << std::endl << std::endl;
		for(std::size_t i=0;i<4;i++){
			if(settings[i] && settings[i]->device() == rs.device() && settings[i]->subdev() == rs.subdev() ){
				m_analyzers[i].input(areas.begin(channel[i]), areas.end(channel[i]));
			}
		}
	}
	void setRunning(bool value) { m_running = value; }
	boost::ptr_vector<Analyzer>& analyzers() { return m_analyzers; }
};

#endif
