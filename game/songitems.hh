#pragma once

#include <set>
#include <vector>
#include <string>
#include <stdexcept>

namespace xmlpp { class Node; class Element; typedef std::vector<Node*>NodeSet; }

/**Exception which will be thrown when loading or
  saving Players fails.*/
struct SongItemsException: public std::runtime_error {
	SongItemsException (std::string const& msg) :
		runtime_error(msg)
	{}
};

struct SongItem
{
	int id; // TODO use a PUID instead (LibOFA)

	std::string artist;
	std::string title;

	bool operator< (SongItem const& other) const
	{
		return id < other.id;
	}
};

struct SongItems
{
	void load(xmlpp::NodeSet const& n);
	void save(xmlpp::Element *players);

	void addSong(std::string const& artist, std::string const& title, int id = -1);

private:
	int assign_id_internal();

	typedef std::set<SongItem> songs_t;
	songs_t m_songs;
};
