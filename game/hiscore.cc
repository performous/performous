#include "hiscore.hh"

#include "configuration.hh"
#include "libxml++-impl.hh"


#include <algorithm>
#include <sstream>
#include <stdexcept>

const unsigned Hiscore::MaximumScorePoints = 10000;

bool Hiscore::reachedHiscore(unsigned score, SongId songid, unsigned short level, std::string const& track) const {
	if (score > MaximumScorePoints) {
		throw std::logic_error("Invalid score value, maximum is " + std::to_string(MaximumScorePoints) + " but got " + std::to_string(score));
	}
	if (score < config["game/highscore_minimum_recognized_score_points"].ui()) {
		return false; // come on, did you even try to sing?
	}

	HiscoreItemBySongAndLevelKey const& key = { songid, level };
	if (m_hiscore_map_with_level.find(key) == m_hiscore_map_with_level.end())
		return true;

	unsigned position = 0;
	for (auto const& elem: m_hiscore_map_with_level.at(key)) {
		if (elem.track != track) continue;
		if (score > elem.score) return true;
		if (config["game/highscore_limit_stored_scores"].b() && ++position >= config["game/highscore_maximum_stored_scores"].ui())
			return false;
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

	m_hiscore_map[item.songid].insert(item);
	m_hiscore_map_with_level[{item.songid, item.level}].insert(item);
}

void Hiscore::addHiscoreUnconditionally(HiscoreItem&& item) {
	if (item.track.empty())
		throw std::runtime_error("No track given");


	m_hiscore_map[item.songid].insert(item);
	m_hiscore_map_with_level[{item.songid, item.level}].insert(item);
}

unsigned Hiscore::getHiscore(const SongId songid) const {
	HiscoreItemBySongAndLevelKey const& key = { songid, currentLevel() };
	if ((m_hiscore_map_with_level.find(key) == m_hiscore_map_with_level.end()))
		return 0;

	return  m_hiscore_map_with_level.at(key).begin()->score;
}

Hiscore::HiscoreVector Hiscore::getHiscores(SongId songid) const {
	HiscoreVector hv;

	HiscoreItemBySongAndLevelKey const& key = { songid, currentLevel() };
	if ((m_hiscore_map_with_level.find(key) == m_hiscore_map_with_level.end()))
		return hv;

	auto const& from_map = m_hiscore_map_with_level.at(key);
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

		addHiscoreUnconditionally({score, playerid, songid, level, a_track ? a_track->get_value() : "vocals", unixtime});
	}
}

void Hiscore::save(xmlpp::Element *hiscores) {
	std::vector<HiscoreItem> hiscore_items;
	for (auto const& [song_id, song_hiscores] : m_hiscore_map) {
		hiscore_items.insert(hiscore_items.end(), song_hiscores.begin(), song_hiscores.end());
	}

	std::stable_sort(hiscore_items.begin(), hiscore_items.end(),
	[&](HiscoreItem const& a, HiscoreItem const& b) { return b.unixtime < a.unixtime; });

	for (auto const& hiscore_item: hiscore_items) {
		xmlpp::Element* hiscore = xmlpp::add_child_element(hiscores, "hiscore");
		hiscore->set_attribute("playerid", std::to_string(hiscore_item.playerid));
		hiscore->set_attribute("songid", std::to_string(hiscore_item.songid));
		hiscore->set_attribute("track", hiscore_item.track);
		hiscore->set_attribute("level", std::to_string(hiscore_item.level));
		hiscore->set_attribute("unixtime", std::to_string(hiscore_item.unixtime.count()));
		hiscore->add_child_text(std::to_string(hiscore_item.score));
	}
}

unsigned short Hiscore::currentLevel() const {
	return config["game/difficulty"].ui();
}

// to count all hiscores sum up the sizes of all songs' hiscores
std::size_t Hiscore::size() const {
	std::vector<size_t> sizes(m_hiscore_map.size(), 0);
	auto map_func = [](const std::pair<SongId, hiscore_t>& songHiscores) {return songHiscores.second.size();};
	std::transform(m_hiscore_map.begin(), m_hiscore_map.end(), sizes.begin(), map_func);
	return std::size_t(std::accumulate(sizes.begin(), sizes.end(), 0, std::plus{}));
}
