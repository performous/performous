#include "players.hh"

#include <fstream>
#include <boost/regex.hpp>
#include <libxml++/libxml++.h>

Players::Players(std::string filename):
	m_players(),
	m_filtered(),
	m_filename(filename),
	m_filter(),
	math_cover(),
	m_dirty(false),
	cur()
{
	load();
	m_filtered = m_players;
}

Players::~Players() {
	save();
}

void Players::load() {
	xmlpp::DomParser domParser(m_filename);
	xmlpp::NodeSet n = domParser.get_document()->get_root_node()->find("/performous/players/player");

	for (xmlpp::NodeSet::const_iterator it = n.begin(); it != n.end(); ++it)
	{
		xmlpp::Element& element = dynamic_cast<xmlpp::Element&>(**it);
		xmlpp::Attribute* a = element.get_attribute("name");
		addPlayer(a->get_value());
	}
}

void Players::save() {
	xmlpp::Document doc;
	xmlpp::Node* nodeRoot = doc.create_root_node("performous");
	xmlpp::Element *players = nodeRoot->add_child("players");

	for (players_t::const_iterator it = m_players.begin(); it!=m_players.end(); ++it)
	{
		xmlpp::Element* player = players->add_child("player");
		player->set_attribute("name", it->name);
	}

	doc.write_to_file_formatted(m_filename, "UTF-8");
}

void Players::update() {
	if (m_dirty) filter_internal();
}

void Players::addPlayer (std::string const& name) {
	PlayerItem pi;
	pi.name = name;

	players_t::const_iterator it =  std::find(m_players.begin(), m_players.end(), pi);
	if (it != m_players.end()) return; // dont do anything, player exists

	m_dirty = true;
	m_players.push_back(pi);
}

void Players::setFilter(std::string const& val) {
	if (m_filter == val) return;
	m_filter = val;
	filter_internal();
}

void Players::filter_internal() {
	m_dirty = false;
	PlayerItem selection = current();

	try {
		players_t filtered;
		for (players_t::const_iterator it = m_players.begin(); it != m_players.end(); ++it) {
			if (regex_search(it->name, boost::regex(m_filter, boost::regex_constants::icase))) filtered.push_back(*it);
		}
		m_filtered.swap(filtered);
	} catch (...) {
		players_t(m_players.begin(), m_players.end()).swap(m_filtered);  // Invalid regex => copy everything
	}
	math_cover.reset();

	// Restore old selection
	int pos = 0;
	if (selection.name != "") {
		players_t::iterator it = std::find(m_filtered.begin(), m_filtered.end(), selection);
		math_cover.setTarget(0, 0);
		if (it != m_filtered.end()) pos = it - m_filtered.begin();
	}
	math_cover.setTarget(pos, size());
}


/*
#include <iostream>

int main(int argc, char** argv)
{
	Players p("players.xml");
	p.addPlayer("hubert");

	for (size_t i=0; i<p.size(); i++)
	{
		std::cout << p[i].name << std::endl;
	}

	return 0;
}
*/
