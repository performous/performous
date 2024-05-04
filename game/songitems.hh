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
class SongItems {
public:
	void load(xmlpp::NodeSet const& n);
	void save(xmlpp::Element* songs);

	/**Adds a song item.
	  If the id does not exist or is not unique, a new one will be assigned.
	  There will be no check if artist and title already exist - if you
	  need that you want addSong().
	 */
	SongId addSongItem(std::string const& artist, std::string const& title, std::optional<SongId> id = std::nullopt);
	/**Adds a song to the database.

	  The ID will be assigned.
	  If there is already a song with the same artist and title the existing
	  will be used.
	  @return the ID which has been determined for the given song.
	  */
	SongId addSong(SongPtr song);


	/**Lookup the ID for a specific song.
	  @return the song ID if a song could be found.
	  */
	std::optional<SongId> resolveToSongId(Song const& song) const;

	/**Lookup the SongItem for a specific song id.
	  @return the SongItem for the specified song id.
	  */
	SongItem getSongItemById(SongId const& id) const;

	/**Fetch all SongItems.
	  @return all SongItems.
	  */
	std::unordered_map<SongId, SongItem> getSongItems() const;

	std::size_t size() const { return m_songs_map.size(); }

private:
	SongId assign_id_internal() const;
	bool match_artist_and_title_internal(Song const& song, SongItem const& songItem) const;

	using songs_map_t = std::unordered_map<SongId, SongItem>;
	songs_map_t m_songs_map;
};
