#ifndef __RECORD_H_
#define __RECORD_H_

#include <audio.hpp>
#include "pitch.hh"
#include <boost/ptr_container/ptr_vector.hpp>
#include <iostream>

class CaptureDeviceCallback {
  private:
	std::size_t channels;
	da::settings *settings;
	da::record *record;
	boost::ptr_vector<Analyzer> myanalyzers;
  public:
	CaptureDeviceCallback(std::size_t ch, std::size_t rate, std::string device){
		channels = ch;
		settings = new da::settings(device);
		settings->set_callback(boost::ref(*this));
		settings->set_channels(ch);
		settings->set_rate(rate);
		settings->set_debug(std::cerr);
		record = new da::record(*settings);
		for(std::size_t i=0;i<ch;i++){
			myanalyzers.push_back(new Analyzer(settings->rate()));
		}
		//std::cout << "CaptureDeviceCallback constructed, channels=" << channels << std::endl;
	}
	void operator()(da::pcm_data& areas, da::settings const&) {
		for(std::size_t ch = 0; ch < channels; ++ch){
			myanalyzers[ch].input(areas.begin(ch), areas.end(ch));
		}
	}
	boost::ptr_vector<Analyzer>& analyzers() { return myanalyzers; }
};

class Capture {
  private:
	boost::ptr_vector<Analyzer> all_analyzers;
  public:
	Capture(){}
	void addMics(std::size_t channels, std::size_t rate, std::string device){
		CaptureDeviceCallback *cdc = new CaptureDeviceCallback(channels,rate,device);
		boost::ptr_vector<Analyzer>& lyzer = cdc->analyzers();
		for(std::size_t i=0;i<lyzer.size();i++){
			all_analyzers.push_back(&lyzer[i]);
		}
	}
	boost::ptr_vector<Analyzer>& analyzers() { return all_analyzers; }
};

#endif
