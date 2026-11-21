#include "gamerecorder.hh"

#include "log.hh"

#include <fmt/format.h>

#include <fstream>
#include <iterator>
#include <stdexcept>

GameRecorder::GameRecorder(): m_start(Clock::now()) {
	m_entries.reserve(100000);
}

void GameRecorder::frame(double songTime) {
	m_entries.push_back({'f', songTime, Seconds(Clock::now() - m_start).count(), 0});
}

void GameRecorder::pressed(unsigned num, bool value) {
	m_entries.push_back({'p', value ? 1.0 : 0.0, 0.0, num});
}

void GameRecorder::input(unsigned button, double value, double whammy) {
	m_entries.push_back({'i', value, whammy, button});
}

void GameRecorder::draw(double songTime) {
	m_entries.push_back({'d', songTime, Seconds(Clock::now() - m_start).count(), 0});
}

void GameRecorder::state(double score, int streak, int longestStreak, double starmeter) {
	if (score == m_score && streak == m_streak && longestStreak == m_longestStreak && starmeter == m_starmeter) return;
	m_score = score;
	m_streak = streak;
	m_longestStreak = longestStreak;
	m_starmeter = starmeter;
	m_entries.push_back({'s', score, starmeter, static_cast<unsigned>(streak) << 16 | static_cast<unsigned>(longestStreak)});
}

void GameRecorder::chart(std::string const& track, unsigned level) {
	unsigned index = 0;
	while (index < m_charts.size() && !(m_charts[index].track == track && m_charts[index].level == level)) ++index;
	if (index == m_charts.size()) m_charts.push_back({track, level});
	m_entries.push_back({'c', 0.0, 0.0, index});
}

void GameRecorder::rejoin() {
	m_entries.push_back({'j', 0.0, 0.0, 0});
}

nlohmann::json GameRecorder::entries() const {
	auto out = nlohmann::json::array();
	for (auto const& e: m_entries) {
		switch (e.type) {
		  case 'f': out.push_back({"f", e.time, e.wall}); break;
		  case 'p': out.push_back({"p", e.arg, e.time != 0.0}); break;
		  case 'i': out.push_back({"i", e.arg, e.time, e.wall}); break;
		  case 'd': out.push_back({"d", e.time, e.wall}); break;
		  case 's': out.push_back({"s", e.time, e.arg >> 16, e.arg & 0xFFFF, e.wall}); break;
		  case 'c': out.push_back({"c", e.arg}); break;
		  case 'j': out.push_back({"j"}); break;
		  default: break;
		}
	}
	return out;
}

nlohmann::json chartToJson(InstrumentTrack const& track, unsigned basepitch) {
	std::vector<unsigned> pitches;
	for (unsigned fret = 0; fret < 5; ++fret) pitches.push_back(basepitch + fret);
	for (unsigned pitch: {103u, 110u, 111u, 112u, 124u}) pitches.push_back(pitch);  // Solos, toms, drum fills
	auto out = nlohmann::json::object();
	for (unsigned pitch: pitches) {
		auto it = track.nm.find(pitch);
		if (it == track.nm.end()) continue;
		auto notes = nlohmann::json::array();
		for (auto const& d: it->second) notes.push_back({d.begin, d.end});
		out[std::to_string(pitch)] = notes;
	}
	return out;
}

std::string songFingerprint(fs::path const& notesFile) {
	// FNV-1a: stable across platforms and builds, enough to match a song a maintainer owns
	std::ifstream f(notesFile, std::ios::binary);
	if (!f) return {};
	std::uint64_t hash = 14695981039346656037ull;
	for (std::istreambuf_iterator<char> it(f), end; it != end; ++it) {
		hash ^= static_cast<unsigned char>(*it);
		hash *= 1099511628211ull;
	}
	return fmt::format("fnv1a64:{:016x}", hash);
}

fs::path saveGameReport(nlohmann::json const& report) {
	fs::path dir = PathCache::getDataDir() / "reports";
	fs::create_directories(dir);
	fs::path file;
	for (unsigned n = 1; ; ++n) {
		file = dir / fmt::format("performous-report-{}.json", n);
		if (!fs::exists(file)) break;
	}
	std::ofstream out(file);
	if (!out) throw std::runtime_error("Cannot write " + file.string());
	out << report.dump() << '\n';
	SpdLogger::notice(LogSystem::INSTRUMENTS, "Game report saved, path={}", file);
	return file;
}
