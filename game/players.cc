#include "players.hh"

#include <fstream>
#include <boost/regex.hpp>

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
	std::ifstream in (m_filename.c_str());

	std::string str;
	while (getline(in, str))
	{
		addPlayer(str);
	}
}

void Players::save() {
	std::ofstream out (m_filename.c_str());

	for (players_t::const_iterator it = m_players.begin(); it!=m_players.end(); ++it)
	{
		out << it->name << std::endl;
	}
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

#include <iostream> //TODO remove

void Players::filter_internal() {
	m_dirty = false;

	if (m_filter == "")
	{
		std::cout << "empty filter" << std::endl; //TODO remove
		// without filter get all names
		m_filtered = m_players;
		return;
	}

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
	// TODO restore selection
}


/*
#include <iostream>

int main(int argc, char** argv)
{
	if (argc != 2) return 1;

	std::string filter = argv[1];
	Players p("players.txt");
	p.addPlayer("hubert");
	p.setFilter(filter);

	for (size_t i=0; i<p.size(); i++)
	{
		std::cout << p[i].name << std::endl;
	}
}
*/
