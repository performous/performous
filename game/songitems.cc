#include "songitems.hh"

#include "unicode.hh"
#include "libxml++-impl.hh"

#include <algorithm>
#include <memory>
#include <string>

void SongItems::load(xmlpp::NodeSet const& n) {
	for (auto const& elem: n) {
		xmlpp::Element& element = dynamic_cast<xmlpp::Element&>(*elem);

		xmlpp::Attribute* a_id = element.get_attribute("id");
		if (!a_id)
			throw SongItemsException("No attribute id");
		auto id = std::stoi(a_id->get_value());

		xmlpp::Attribute* a_artist = element.get_attribute("artist");
		if (!a_artist)
			throw SongItemsException("No attribute artist");
		auto artist = a_artist->get_value();

		xmlpp::Attribute* a_title = element.get_attribute("title");
		if (!a_title)
			throw SongItemsException("No attribute title");
		auto title = a_title->get_value();

		addSongItem(artist, title, id);
	}
}

void SongItems::save(xmlpp::Element* songs) {
    std::vector<SongItem> song_items;

    std::transform(
	    m_songs_map.begin(),
	    m_songs_map.end(),
	    std::back_inserter(song_items),
	    [](auto &kv) { return kv.second; }
    );

	std::stable_sort(song_items.begin(), song_items.end(),
	[&](SongItem const& a, SongItem const& b) { return a.id < b.id; });

	for (auto const& song : song_items) {
		xmlpp::Element* element = xmlpp::add_child_element(songs, "song");
		element->set_attribute("id", std::to_string(song.id));
		element->set_attribute("artist", song.artist);
		element->set_attribute("title", song.title);
	}
}

SongId SongItems::addSongItem(std::string const& artist, std::string const& title, std::optional<SongId> _id) {
	SongItem si;

	si.id = _id.has_value() ? _id.value() : assign_id_internal();

	si.artist = artist;
	si.title = title;

	m_songs_map[si.id] = si;
	return si.id;
}

SongId SongItems::addSong(SongPtr song) {
	// if song is already in db verify integrity and return
	if (song->id.has_value() && m_songs_map.find(song->id.value()) != m_songs_map.end()) {
		// verify artist and title match
		if (match_artist_and_title_internal(*song, m_songs_map.at(song->id.value())))
			return song->id.value();
		// else the song has a wrong ID and should take on whatever ID is assigned during addSongItem
	}

	// if the song can be resolved to an ID that means there is an entry for it in the DB -> no new song will be added
	auto const& maybe_id = resolveToSongId(*song);

	// Do NOT use .value_or() here; it gets evaluated and addSongItem() runs regardless of whether we have a value, which results in duplicate entries in the database.
	return maybe_id.has_value() ? maybe_id.value() : addSongItem(song->artist, song->title);
}

std::optional<SongId> SongItems::resolveToSongId(Song const& song) const {
	auto const si = std::find_if(m_songs_map.begin(), m_songs_map.end(), [&song](std::pair<const SongId, SongItem> const& item) {
		return match_artist_and_title_internal(song, item.second);
	});
	if (si != m_songs_map.end())
		return si->second.id;
	return std::nullopt;
}

bool SongItems::match_artist_and_title_internal(Song const& song, SongItem const& songItem) {
	// This is not always really correct but in most cases these inputs should have been normalized into unicode at one point during their life time.
	if (song.artist.length() != songItem.artist.length() || song.title.length() != songItem.title.length())
		return false;

	return UnicodeUtil::caseEqual(song.artist, songItem.artist, true) && UnicodeUtil::caseEqual(song.title, songItem.title, true);
}

SongId SongItems::assign_id_internal() const {
	// use the last one with highest id
	auto it = std::max_element(m_songs_map.begin(), m_songs_map.end());
	if (it != m_songs_map.end()) return it->second.id+1;
	return 0; // empty map
}
