#include "players.hh"

#include <fstream>

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

#include <iostream>

int main(int argc, char** argv)
{
	if (argc != 2) return 1;

	std::string name = argv[1];
	Players p("players.txt");
	p.load();
	p.addPlayer(name);
	p.save();
}
