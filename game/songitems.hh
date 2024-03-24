#pragma once

#include "song.hh"

#include "libxml++.hh"

#include <memory>
#include <set>
#include <vector>
#include <string>
#include <stdexcept>
#include <optional>

/**Exception which will be thrown when loading or
  saving Players fails.*/
struct SongItemsException: public std::runtime_error {
	SongItemsException (std::string const& msg) :
		runtime_error(msg)
	{}
};

using SongId = unsigned;

struct SongItem
{
	SongId id = 0; ///< The unique id for every song

	/** This data is stored separate because it is read in before
	  the song is added.
	  A short, but relatively non-ambiguous collate form is used.
	  */
	std::string artist;
	std::string title;

	std::shared_ptr<Song> getSong() const;
	void setSong(std::shared_ptr<Song>);

	bool isBroken() const;
	void setBroken(bool broken = true);

	bool operator< (SongItem const& other) const
	{
		return id < other.id;
	}

private:
	bool m_broken = false;
	std::shared_ptr<Song> m_song;
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
class SongItems {
public:
	void load(xmlpp::NodeSet const& n);
	void save(xmlpp::Element *players);

	/**Adds a song item.
	  If the id does not exist or is not unique, a new one will be assigned.
	  There will be no check if artist and title already exist - if you
	  need that you want addSong().
	 */
	SongId addSongItem(std::string const& artist, std::string const& title, bool broken = false, std::optional<SongId> id = std::nullopt);
	/**Adds or Links an already existing song with an songitem.

	  The id will be assigned and artist and title will be filled in.
	  If there is already a song with the same artist and title the existing
	  will be used.

	  Afterwards the pointer to the song will be stored for entire available
	  song information.

	  lookup is used internally to achieve that.
	  */
	void addSong(std::shared_ptr<Song> song);

	/**Lookup a songid for a specific song.
	  @return a value only if a song was found.*/
	std::optional<SongId> lookup(std::shared_ptr<Song> song) const { if (song) return lookup(*song); return std::nullopt; };
	std::optional<SongId> lookup(Song const& song) const;

	SongId getSongId(SongPtr const&) const;
	SongPtr getSong(SongId) const;

	/**Lookup the artist + title for a specific song.
	  @return "Unknown Song" if nothing is found.
	  */
	std::optional<std::string> lookup (const SongId& id) const;

	std::size_t size() const { return m_songs.size(); }

private:
	SongId assign_id_internal() const;

	using songs_t = std::set<SongItem>;
	songs_t m_songs;
};
