#ifndef __RECORD_H_
#define __RECORD_H_

#include <audio.hpp>
#include "pitch.hh"
#include <iostream>

class Capture {
	static const std::size_t DEFAULT_RATE = 48000;
	Analyzer m_analyzer;
	da::settings m_rs;
	da::record m_record;
  public:
	Capture(std::string const& device = "", std::size_t rate = DEFAULT_RATE):
	  m_rs(da::settings(device)
	  .set_callback(boost::ref(*this))
	  .set_channels(1)
	  .set_rate(rate)
	  .set_debug(std::cerr)),
	  m_record(m_rs)
	{
		m_analyzer.setRate(m_rs.rate());
	}
	void operator()(da::pcm_data& areas, da::settings const&) {
		m_analyzer.input(areas.begin(0), areas.end(0));
	}
	Analyzer const& analyzer() const { return m_analyzer; }
};

#endif
