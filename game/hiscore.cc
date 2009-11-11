#include "hiscore.hh"

#include <sstream>
#include <algorithm>

#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <libxml++/libxml++.h>

Hiscore::Hiscore()
{}

void Hiscore::addHiscore(int score, int playerid, int songid, std::string const& track)
{
	HiscoreItem hi;
	hi.score = score;

	hi.playerid = playerid;
	hi.songid = songid;

	hi.track = track;

	m_hiscore.insert(hi);
}

void Hiscore::load(xmlpp::NodeSet n)
{
	for (xmlpp::NodeSet::const_iterator it = n.begin(); it != n.end(); ++it)
	{
		xmlpp::Element& element = dynamic_cast<xmlpp::Element&>(**it);
		xmlpp::Attribute* a_playerid = element.get_attribute("playerid");
		xmlpp::Attribute* a_songid = element.get_attribute("songid");
		xmlpp::Attribute* a_track = element.get_attribute("track");

		int playerid = boost::lexical_cast<int>(a_playerid->get_value());
		int songid = boost::lexical_cast<int>(a_songid->get_value());

		xmlpp::TextNode* tn = element.get_child_text();
		int score = boost::lexical_cast<int>(tn->get_content());
		if (score < 0) throw boost::numeric::negative_overflow();
		if (score > 10000) throw boost::numeric::positive_overflow();

		std::string track = a_track->get_value();

		addHiscore(score, playerid, songid, track);
	}
}

void Hiscore::save(xmlpp::Element *hiscores)
{
	for (hiscore_t::const_iterator it = m_hiscore.begin(); it != m_hiscore.end(); ++it)
	{
		xmlpp::Element* hiscore = hiscores->add_child("hiscore");
		hiscore->set_attribute("playerid", boost::lexical_cast<std::string>(it->playerid));
		hiscore->set_attribute("songid", boost::lexical_cast<std::string>(it->songid));
		hiscore->set_attribute("track", it->track);
		hiscore->add_child_text(boost::lexical_cast<std::string>(it->score));
	}
}
