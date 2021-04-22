#include "hiscore.hh"

#include "configuration.hh"
#include "libxml++-impl.hh"


#include <algorithm>
#include <sstream>
#include <stdexcept>

bool Hiscore::reachedHiscore(unsigned score, unsigned songid, unsigned level, std::string const& track) const {
	if (score > 10000) throw std::logic_error("Invalid score value");
	if (score < 2000) return false; // come on, did you even try to sing?

	unsigned position = 0;
	for (auto const& elem: m_hiscore) {
		if (elem.songid != songid) continue;
		if (elem.track != track) continue;
		if (elem.level != level) continue;
		if (score > elem.score) return true; // seems like you are in top 3!
		if (++position == 3) return false; // not in top 3 -> leave
	}
	return true; // nothing found for that song -> true
}

void Hiscore::addHiscore(unsigned score, unsigned playerid, unsigned songid, unsigned level, std::string const& track) {
	if (track.empty()) throw std::runtime_error("No track given");
	if (!reachedHiscore(score, songid, level, track)) return;
	m_hiscore.insert(HiscoreItem(score, playerid, songid, level, track));
}

Hiscore::HiscoreVector Hiscore::queryHiscore(unsigned max, unsigned playerid, unsigned songid, std::string const& track) const {
	HiscoreVector hv;
	for (auto const& h: m_hiscore) {
		if (playerid != unsigned(-1) && playerid != h.playerid) continue;
		if (songid != unsigned(-1) && songid != h.songid) continue;
		if (currentLevel() != h.level) continue;
		if (!track.empty() && track != h.track) continue;
		if (--max == 0) break;
		hv.push_back(h);
	}
	return hv;
}

bool Hiscore::hasHiscore(unsigned songid) const {
	for (auto const& h: m_hiscore) if (songid == h.songid && currentLevel()==h.level) return true;
	return false;
}

void Hiscore::load(xmlpp::NodeSet const& nodes) {
	for (auto const& n: nodes) {
		xmlpp::Element& element = dynamic_cast<xmlpp::Element&>(*n);
		xmlpp::Attribute* a_playerid = element.get_attribute("playerid");
		if (!a_playerid) throw std::runtime_error("Attribute playerid not found");
		xmlpp::Attribute* a_songid = element.get_attribute("songid");
		if (!a_songid) throw std::runtime_error("Attribute songid not found");
		xmlpp::Attribute* a_track = element.get_attribute("track");
		xmlpp::Attribute* a_level = element.get_attribute("level");



		int playerid = std::stoi(a_playerid->get_value());
		int songid = std::stoi(a_songid->get_value());
		unsigned level = 0;
		if (a_level)
			level = std::stoi(a_level->get_value());

		auto tn = xmlpp::get_first_child_text(element);
		if (!tn) throw std::runtime_error("Score not found");
		int score = std::stoi(tn->get_content());

		addHiscore(score, playerid, songid, level, a_track ? a_track->get_value() : "vocals");
	}
}

void Hiscore::save(xmlpp::Element *hiscores) {
	for (auto const& h: m_hiscore) {
		xmlpp::Element* hiscore = xmlpp::add_child_element(hiscores, "hiscore");
		hiscore->set_attribute("playerid", std::to_string(h.playerid));
		hiscore->set_attribute("songid", std::to_string(h.songid));
		hiscore->set_attribute("track", h.track);
		hiscore->set_attribute("level", std::to_string(h.level));
		hiscore->add_child_text(std::to_string(h.score));
	}
}

int Hiscore::currentLevel() const {
	return config["game/difficulty"].i();
}
