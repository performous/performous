#include "engine.hh"

#include "audio.hh"
#include "configuration.hh"

const double Engine::TIMESTEP = 0.01;

void Engine::operator()() {
	while (!m_quit) {
		std::for_each(m_database.cur.begin(), m_database.cur.end(), boost::bind(&Player::prepare, _1));
		double t = m_audio.getPosition() - config["audio/round-trip"].f();
		double timeLeft = m_time * TIMESTEP - t;
		if (timeLeft != timeLeft || timeLeft > 1.0) timeLeft = 1.0;  // FIXME: Workaround for NaN values and other weirdness (should fix the weirdness instead)
		if (timeLeft > 0.0) { boost::thread::sleep(now() + std::min(TIMESTEP, timeLeft)); continue; }
		std::for_each(m_database.cur.begin(), m_database.cur.end(), boost::bind(&Player::update, _1));
		++m_time;
	}
}
