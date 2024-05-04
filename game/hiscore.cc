#include "hiscore.hh"

#include "configuration.hh"
#include "libxml++-impl.hh"


#include <algorithm>
#include <sstream>
#include <stdexcept>

const unsigned Hiscore::MaximumScorePoints = 10000;
const unsigned Hiscore::MinimumRecognizedScorePoints = 2000;
const unsigned Hiscore::MaximumStoredScores = 16;

bool Hiscore::reachedHiscore(unsigned score, SongId songid, unsigned short level, std::string const& track) const {
	if (score > MaximumScorePoints) {
		throw std::logic_error("Invalid score value, maximum is " + std::to_string(MaximumScorePoints) + " but got " + std::to_string(score));
	}
	if (score < MinimumRecognizedScorePoints) {
		return false; // come on, did you even try to sing?
	}

	unsigned position = 0;
	for (auto const& elem: m_hiscore) {
		if (elem.songid != songid) continue;
		if (elem.track != track) continue;
		if (elem.level != level) continue;
		if (score > elem.score) return true;
		if (++position >= MaximumStoredScores) return false;
	}
	return true; // nothing found for that song -> true
}

void Hiscore::addHiscore(unsigned score, const PlayerId& playerid, SongId songid, unsigned short level, std::string const& track) {
	addHiscore({score, playerid, songid, level, track});
}

void Hiscore::addHiscore(HiscoreItem&& item) {
	if (item.track.empty())
		throw std::runtime_error("No track given");
	if (!reachedHiscore(item.score, item.songid, item.level, item.track))
		return;

	m_hiscore.insert(item);
	m_hiscore_map[item.songid].insert(item);
	m_hiscore_map_with_level[{item.songid, item.level}].insert(item);
}

Hiscore::HiscoreVector Hiscore::queryHiscore(std::optional<PlayerId> playerid, std::optional<SongId> songid, std::string const& track, std::optional<unsigned> max) const {
	HiscoreVector hv;
	for (auto const& h: m_hiscore) {
		if (playerid && playerid.value() != h.playerid) continue;
		if (songid && songid.value() != h.songid) continue;
		if (currentLevel() != h.level) continue;
		if (!track.empty() && track != h.track) continue;
		if (max && --max.value() == 0) break;
		hv.push_back(h);
	}
	return hv;
}

bool Hiscore::hasHiscore(const SongId& songid) const {
	return std::any_of(m_hiscore.begin(),m_hiscore.end(), [&songid, level = currentLevel()](auto const& h) {
		return songid == h.songid && level == h.level;
	});
}

unsigned Hiscore::getHiscore(const SongId songid) const {
	if (m_hiscore_map_with_level.find({ songid, currentLevel() }) != m_hiscore_map_with_level.end()) {
		return  m_hiscore_map_with_level.at({ songid, currentLevel() }).begin()->score;
	}

	return 0;
}

unsigned Hiscore::getHiscoreForLevel(const SongId songid, unsigned short level) const {
	auto const& scores = m_hiscore_map_with_level.at({ songid, level });
	if (scores.empty())
		return 0;

	return scores.begin()->score;
}

Hiscore::HiscoreVector Hiscore::getHiscores(SongId songid) const {
	HiscoreVector hv;
	auto const& from_map = m_hiscore_map_with_level.at({ songid, currentLevel() });
	std::copy(from_map.begin(), from_map.end(), std::back_inserter(hv));

	return hv;
}

size_t Hiscore::getAllHiscoresCount(SongId songid) const {
	return m_hiscore_map.at(songid).size();
}

void Hiscore::load(xmlpp::NodeSet const& nodes) {
	for (auto const& n: nodes) {
		xmlpp::Element& element = dynamic_cast<xmlpp::Element&>(*n);
		xmlpp::Attribute* a_playerid = element.get_attribute("playerid");
		if (!a_playerid) {
			throw std::runtime_error("Attribute playerid not found");
		}
		xmlpp::Attribute* a_songid = element.get_attribute("songid");
		if (!a_songid) {
			throw std::runtime_error("Attribute songid not found");
		}
		xmlpp::Attribute* a_track = element.get_attribute("track");
		xmlpp::Attribute* a_level = element.get_attribute("level");
		xmlpp::Attribute* a_time = element.get_attribute("unixtime");

		PlayerId playerid = stou(a_playerid->get_value());
		SongId songid = stou(a_songid->get_value());
		unsigned short level = 0;
		if (a_level) {
			level = static_cast<unsigned short>(stou(a_level->get_value()));
		}

		auto tn = xmlpp::get_first_child_text(element);
		if (!tn) {
			throw std::runtime_error("Score not found");
		}

		unsigned score = stou(tn->get_content());
		auto unixtime = std::chrono::seconds{};

		if(a_time) {
			unixtime = std::chrono::seconds(stou(a_time->get_value()));
		}

		addHiscore({score, playerid, songid, level, a_track ? a_track->get_value() : "vocals", unixtime});
	}
}

void Hiscore::save(xmlpp::Element *hiscores) {
	for (auto const& h: m_hiscore) {
		xmlpp::Element* hiscore = xmlpp::add_child_element(hiscores, "hiscore");
		hiscore->set_attribute("playerid", std::to_string(h.playerid));
		hiscore->set_attribute("songid", std::to_string(h.songid));
		hiscore->set_attribute("track", h.track);
		hiscore->set_attribute("level", std::to_string(h.level));
		hiscore->set_attribute("unixtime", std::to_string(h.unixtime.count()));
		hiscore->add_child_text(std::to_string(h.score));
	}
}

unsigned short Hiscore::currentLevel() const {
	return config["game/difficulty"].ui();
}
