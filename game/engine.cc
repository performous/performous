#include "engine.hh"

#include "audio.hh"
#include "song.hh"
#include "database.hh"
#include "configuration.hh"
#include <iostream>
#include <list>

const float Engine::TIMESTEP = 0.01f;

Engine::Engine(Audio& audio, VocalTrackPtrs vocals, Database& database):
  m_audio(audio), m_time(), m_quit(), m_database(database)
{
	auto& analyzers = m_audio.analyzers();
	if (analyzers.size() != vocals.size()) throw std::logic_error("Engine requires the same number of vocal tracks as there are analyzers.");
	// Clear old player information
	m_database.cur.clear();
	m_database.scores.clear();
	unsigned i = 0;
	for (Analyzer& a: analyzers) {
		// Calculate the space required for pitch frames
		size_t frames = vocals[i]->endTime / Engine::TIMESTEP;
		m_database.cur.push_back(Player(*vocals[i], a, frames));
		++i;
	}
	m_thread.reset(new std::thread(std::ref(*this)));
}

void Engine::operator()() {
	while (!m_quit) {
		for (Player& player: m_database.cur) player.prepare();
		float t = m_audio.getPosition() - config["audio/round-trip"].f();
		float timeLeft = m_time - t;
		if (timeLeft != timeLeft || timeLeft > 1.0) timeLeft = 1.0;  // FIXME: Workaround for NaN values and other weirdness (should fix the weirdness instead)
		if (timeLeft > 0.0) { std::this_thread::sleep_for(std::min(TIMESTEP, timeLeft) * 1s); continue; }
		for (Player& player: m_database.cur) player.update();
		m_time += TIMESTEP;
	}
}
