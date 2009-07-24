#include "players.hh"

#include <fstream>
#include <boost/regex.hpp>

Players::Players(std::string filename):
	m_filename(filename)
{}

Players::~Players()
{}

void Players::load()
{
	std::ifstream in (m_filename.c_str());

	std::string str;
	while (getline(in, str))
	{
		addPlayer(str);
	}
}

void Players::save()
{
	std::ofstream out (m_filename.c_str());

	for (players_t::const_iterator it = m_players.begin(); it!=m_players.end(); ++it)
	{
		out << it->name << std::endl;
	}
}

void Players::addPlayer (std::string name)
{
	PlayerItem pi;
	pi.name = name;
	m_players.push_back(pi);
}

void Players::setFilter(std::string const& val) {
	if (m_filter == val) return;
	m_filter = val;
	filter_internal();
}

void Players::filter_internal() {
	try {
		players_t filtered;
		for (players_t::const_iterator it = m_players.begin(); it != m_players.end(); ++it) {
			if (regex_search(it->name, boost::regex(m_filter, boost::regex_constants::icase))) filtered.push_back(*it);
		}
		m_filtered.swap(filtered);
	} catch (...) {
		players_t(m_players.begin(), m_players.end()).swap(m_filtered);  // Invalid regex => copy everything
	}
	// TODO: reset images
}

/*
#include <iostream>

int main(int argc, char** argv)
{
	if (argc != 2) return 1;

	std::string filter = argv[1];
	Players p("players.txt");
	p.load();
	// p.addPlayer(name);
	p.setFilter(filter);

	for (size_t i=0; i<p.size(); i++)
	{
		std::cout << p[i].name << std::endl;
	}

	p.save();
}
*/
