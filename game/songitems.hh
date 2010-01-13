#pragma once

#include "song.hh"

#include <set>
#include <vector>
#include <string>
#include <stdexcept>

#include <boost/shared_ptr.hpp>

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
	int id; ///< The unique id for every song

	/** This data is stored separate because it is read in before
	  the song is added.
	  A short, but relatively non-ambiguous collate form is used.
	  */
	std::string artist;
	std::string title;

	/** This shared pointer is stored to access all song
	  information available.
	  E.g. the full artist information can be accessed using
	  this pointer.
	 */
	boost::shared_ptr<Song> song;

	bool operator< (SongItem const& other) const
	{
		return id < other.id;
	}
};

/**A list of songs for the database.

  Every song has a unique id managed by that database.
  This class was introduced to hide the implementation
  detail which data structure is used for the list away.

  Currently a std::set is used, which makes both addSongItem()
  and addSong() slow. The only advantage is that the id is
  unique and it is cheap to get a new unique id.

  When one of the methods is to slow, it can be optimized
  easily. */
struct SongItems
{
	void load(xmlpp::NodeSet const& n);
	void save(xmlpp::Element *players);

	/**Adds a song item.
	  If the id is not unique or -1 a new one will be assigned.
	  There will be no check if artist and title already exist - if you
	  need that you want addSong().
	 */
	int addSongItem(std::string const& artist, std::string const& title, int id = -1);
	/**Adds or Links an already existing song with an songitem.

	  The id will be assigned and artist and title will be filled in.
	  If there is already a song with the same artist and title the existing
	  will be used.

	  Afterwards the pointer to the song will be stored for entire available
	  song information.

	  lookup is used internally to achieve that.
	  */
	void addSong(boost::shared_ptr<Song> song);

	/**Lookup a songid for a specific song.
	  @return -1 if no song found.*/
	int lookup(boost::shared_ptr<Song> song) const;
	int lookup(Song& song) const;

	/**Lookup the artist + title for a specific song.
	  @return "Unknown Song" if nothing is found.
	  */
	std::string lookup (int id) const;

  private:
	int assign_id_internal() const;

	typedef std::set<SongItem> songs_t;
	songs_t m_songs;
};
