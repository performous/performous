#include "hiscore.hh"

#include <sstream>
#include <algorithm>

#include <boost/lexical_cast.hpp>

#include <libxml++/libxml++.h>

Hiscore::Hiscore()
{}

bool Hiscore::reachedHiscore(int score, int songid, std::string const& track) {
	if (score < 0) throw HiscoreException("Score negativ overflow");
	if (score > 10000) throw HiscoreException("Score positive overflow");

	if (score < 500) return false; // come on, did you even try to sing?

	int counter = 0;
	for (hiscore_t::const_iterator it = m_hiscore.begin(); it != m_hiscore.end(); ++it)
	{
		if (it->songid != songid) continue;
		if (it->track != track) continue;
		if (score > it->score) return true; // seems like you are in top 3!
		else ++counter;
		if (counter == 3) return false; // not in top 3 -> leave
	}
	return true; // nothing found for that song -> true
}

void Hiscore::addHiscore(int score, int playerid, int songid, std::string const& track) {
	HiscoreItem hi;
	if (score < 0) throw HiscoreException("Score negativ overflow");
	if (score > 10000) throw HiscoreException("Score positive overflow");
	hi.score = score;

	if (playerid < 0) throw HiscoreException("No player given");
	hi.playerid = playerid;

	if (songid < 0) throw HiscoreException("No song given");
	hi.songid = songid;

	if (track.empty()) throw HiscoreException("No track given");
	hi.track = track;

	m_hiscore.insert(hi);
}

void Hiscore::load(xmlpp::NodeSet const& n) {
	for (xmlpp::NodeSet::const_iterator it = n.begin(); it != n.end(); ++it)
	{
		xmlpp::Element& element = dynamic_cast<xmlpp::Element&>(**it);
		xmlpp::Attribute* a_playerid = element.get_attribute("playerid");
		if (!a_playerid) throw HiscoreException("Attribute playerid not found");
		xmlpp::Attribute* a_songid = element.get_attribute("songid");
		if (!a_songid) throw HiscoreException("Attribute songid not found");
		xmlpp::Attribute* a_track = element.get_attribute("track");

		int playerid = boost::lexical_cast<int>(a_playerid->get_value());
		int songid = boost::lexical_cast<int>(a_songid->get_value());

		xmlpp::TextNode* tn = element.get_child_text();
		if (!tn) throw HiscoreException("Score not found");
		int score = boost::lexical_cast<int>(tn->get_content());

		std::string track;
		if (!a_track) track = "vocals";
		else track = a_track->get_value();

		addHiscore(score, playerid, songid, track);
	}
}

void Hiscore::save(xmlpp::Element *hiscores) {
	for (hiscore_t::const_iterator it = m_hiscore.begin(); it != m_hiscore.end(); ++it)
	{
		xmlpp::Element* hiscore = hiscores->add_child("hiscore");
		hiscore->set_attribute("playerid", boost::lexical_cast<std::string>(it->playerid));
		hiscore->set_attribute("songid", boost::lexical_cast<std::string>(it->songid));
		hiscore->set_attribute("track", it->track);
		hiscore->add_child_text(boost::lexical_cast<std::string>(it->score));
	}
}
