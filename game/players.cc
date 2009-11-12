#include "players.hh"

#include "fs.hh"
#include "configuration.hh"

#include <set>
#include <fstream>
#include <iostream>

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include <libxml++/libxml++.h>

Players::Players():
	m_players(),
	m_filtered(),
	m_filter(),
	math_cover(),
	m_dirty(false)
{ }

Players::~Players()
{ }

void Players::load(xmlpp::NodeSet const& n) {
	for (xmlpp::NodeSet::const_iterator it = n.begin(); it != n.end(); ++it) {
		xmlpp::Element& element = dynamic_cast<xmlpp::Element&>(**it);
		xmlpp::Attribute* a_name = element.get_attribute("name");
		if (!a_name) throw PlayersException("Attribute name not found");
		xmlpp::Attribute* a_id = element.get_attribute("id");
		if (!a_id) throw PlayersException("Attribute id not found");
		int id = -1;
		try {id = boost::lexical_cast<int>(a_id->get_value());} catch (boost::bad_lexical_cast const&) { }
		xmlpp::NodeSet n2 = element.find("picture");
		std::string picture;
		if (!n2.empty()) // optional picture element
		{
			xmlpp::Element& element2 =dynamic_cast<xmlpp::Element&>(**n2.begin());
			xmlpp::TextNode* tn = element2.get_child_text();
			picture = tn->get_content();
		}
		addPlayer(a_name->get_value(), picture, id);
	}
}

void Players::save(xmlpp::Element *players) {
	for (players_t::const_iterator it = m_players.begin(); it!=m_players.end(); ++it) {
		xmlpp::Element* player = players->add_child("player");
		player->set_attribute("name", it->name);
		player->set_attribute("id", boost::lexical_cast<std::string>(it->id));
		if (it->picture != "")
		{
			xmlpp::Element* picture = player->add_child("picture");
			picture->add_child_text(it->picture);
		}
	}
}

void Players::update() {
	if (m_dirty) filter_internal();
}

int Players::lookup(std::string const& name) const {
	for (players_t::const_iterator it = m_players.begin(); it != m_players.end(); ++it) {
		if (it->name == name) return it->id;
	}

	return -1;
}

std::string Players::lookup(int id) const {
	PlayerItem pi;
	pi.id = id;
	players_t::iterator it = m_players.find(pi);
	if (it == m_players.end()) return "Unkown Player";
	else return it->name;
}

void Players::addPlayer (std::string const& name, std::string const& picture, int id) {
	PlayerItem pi;
	pi.id = id;
	pi.name = name;
	pi.picture = picture;
	pi.path = "";


	if (pi.id == -1) pi.id = assign_id_internal();

	if (pi.picture != "") // no picture, so don't search path
	{
		// pi.picture =  getXdgPath(fs::path("pictures") / pi.picture);
	}

	m_dirty = true;
	std::pair<players_t::iterator, bool> ret = m_players.insert(pi);
	if (!ret.second)
	{
		pi.id = assign_id_internal();
		m_players.insert(pi); // now do the insert with the fresh id
	}
}

void Players::setFilter(std::string const& val) {
	if (m_filter == val) return;
	m_filter = val;
	filter_internal();
}

int Players::assign_id_internal() {
	players_t::const_reverse_iterator it = m_players.rbegin();
	if (it != m_players.rend()) return it->id+1;
	else return 1; // empty set
}

void Players::filter_internal() {
	m_dirty = false;
	PlayerItem selection = current();

	try {
		fplayers_t filtered;
		for (players_t::const_iterator it = m_players.begin(); it != m_players.end(); ++it) {
			if (regex_search(it->name, boost::regex(m_filter, boost::regex_constants::icase))) filtered.push_back(*it);
		}
		m_filtered.swap(filtered);
	} catch (...) {
		fplayers_t(m_players.begin(), m_players.end()).swap(m_filtered);  // Invalid regex => copy everything
	}
	math_cover.reset();

	// Restore old selection
	int pos = 0;
	if (selection.name != "") {
		fplayers_t::iterator it = std::find(m_filtered.begin(), m_filtered.end(), selection);
		math_cover.setTarget(0, 0);
		if (it != m_filtered.end()) pos = it - m_filtered.begin();
	}
	math_cover.setTarget(pos, size());
}
