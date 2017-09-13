#include "hiscore.hh"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <libxml++/libxml++.h>

bool Hiscore::reachedHiscore(unsigned score, unsigned songid, std::string const& track) const {
	if (score > 10000) throw std::logic_error("Invalid score value");
	if (score < 2000) return false; // come on, did you even try to sing?

	unsigned position = 0;
	for (auto const& elem: m_hiscore) {
		if (elem.songid != songid) continue;
		if (elem.track != track) continue;
		if (score > elem.score) return true; // seems like you are in top 3!
		if (++position == 3) return false; // not in top 3 -> leave
	}
	return true; // nothing found for that song -> true
}

void Hiscore::addHiscore(unsigned score, unsigned playerid, unsigned songid, std::string const& track) {
	if (track.empty()) throw std::runtime_error("No track given");
	if (!reachedHiscore(score, songid, track)) return;
	m_hiscore.insert(HiscoreItem(score, playerid, songid, track));
}

Hiscore::HiscoreVector Hiscore::queryHiscore(unsigned max, unsigned playerid, unsigned songid, std::string const& track) const {
	HiscoreVector hv;
	for (auto const& h: m_hiscore) {
		if (playerid != unsigned(-1) && playerid != h.playerid) continue;
		if (songid != unsigned(-1) && songid != h.songid) continue;
		if (!track.empty() && track != h.track) continue;
		if (--max == 0) break;
		hv.push_back(h);
	}
	return hv;
}

bool Hiscore::hasHiscore(unsigned songid) const {
	for (auto const& h: m_hiscore) if (songid == h.songid) return true;
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

		int playerid = boost::lexical_cast<int>(a_playerid->get_value());
		int songid = boost::lexical_cast<int>(a_songid->get_value());

		xmlpp::TextNode* tn = element.get_child_text();
		if (!tn) throw std::runtime_error("Score not found");
		int score = boost::lexical_cast<int>(tn->get_content());

		addHiscore(score, playerid, songid, a_track ? a_track->get_value() : "vocals");
	}
}

void Hiscore::save(xmlpp::Element *hiscores) {
	for (auto const& h: m_hiscore) {
		xmlpp::Element* hiscore = hiscores->add_child("hiscore");
		hiscore->set_attribute("playerid", boost::lexical_cast<std::string>(h.playerid));
		hiscore->set_attribute("songid", boost::lexical_cast<std::string>(h.songid));
		hiscore->set_attribute("track", h.track);
		hiscore->add_child_text(std::to_string(h.score));
	}
}
