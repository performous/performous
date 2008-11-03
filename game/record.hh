#ifndef __RECORD_H_
#define __RECORD_H_

#include <audio.hpp>
#include "pitch.hh"
#include <boost/ptr_container/ptr_vector.hpp>
#include <iostream>

class Capture {
	static const std::size_t DEFAULT_RATE = 48000;
	// The order of the following variables is important!
	volatile bool m_running;
	boost::ptr_vector<Analyzer> m_analyzers;
	da::settings m_rs;
	da::record m_record;
  public:
	Capture(std::size_t channels = 2, std::string const& device = "", std::size_t rate = DEFAULT_RATE):
	  m_running(false),
	  m_rs(da::settings(device)
	  .set_callback(boost::ref(*this))
	  .set_channels(channels)
	  .set_rate(rate)
	  .set_debug(std::cerr)),
	  m_record(m_rs)
	{
		for (std::size_t ch = 0; ch < channels; ++ch) {
			m_analyzers.push_back(new Analyzer(m_rs.rate()));
		}
		m_running = true;
	}
	void operator()(da::pcm_data& areas, da::settings const&) {
		if (!m_running) return;
		for (std::size_t ch = 0; ch < m_analyzers.size(); ++ch) m_analyzers[ch].input(areas.begin(ch), areas.end(ch));
	}
	void setRunning(bool value) { m_running = value; }
	boost::ptr_vector<Analyzer>& analyzers() { return m_analyzers; }
};

#endif
