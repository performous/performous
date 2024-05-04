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
	for (auto const& [song_id, song] : m_songs_map) {
		xmlpp::Element* element = xmlpp::add_child_element(songs, "song");
		element->set_attribute("id", std::to_string(song.id));
		element->set_attribute("artist", song.artist);
		element->set_attribute("title", song.title);
	}
}

SongId SongItems::addSongItem(std::string const& artist, std::string const& title, std::optional<SongId> _id) {
	SongItem si;

	if (_id.has_value() && m_songs_map.find(_id.value()) != m_songs_map.end()) {
		si = m_songs_map.at(_id.value());
	}

	si.id = _id.value_or(assign_id_internal());
	songMetadata collateInfo {{"artist", artist}, {"title", title}};
	UnicodeUtil::collate(collateInfo);
	si.artist = collateInfo["artist"];
	si.title = collateInfo["title"];

	m_songs_map[si.id] = si;
	return si.id;
}

SongId SongItems::addSong(SongPtr song) {
	auto song_id = resolveToSongId(*song);
	song_id = song_id < 0 ? (addSongItem(song->artist, song->title)) : song_id;

	SongItem si = m_songs_map.at(song_id);

	m_songs_map[si.id] = si;

	return si.id;
}

int SongItems::resolveToSongId(Song const& song) const {
	auto const si = std::find_if(m_songs_map.begin(), m_songs_map.end(), [&song](std::pair<const SongId, SongItem> const& si) {

	// This is not always really correct but in most cases these inputs should have been normalized into unicode at one point during their life time.
	if (song.collateByArtistOnly.length() != si.second.artist.length() ||
		song.collateByTitleOnly.length() != si.second.title.length()) return false;
		return UnicodeUtil::caseEqual(song.collateByArtistOnly, si.second.artist, true) && UnicodeUtil::caseEqual(song.collateByTitleOnly, si.second.title, true);
	});
	if (si != m_songs_map.end()) return si->second.id;
	return -1;
}

SongItem SongItems::getSongItemById(SongId const& id) const {
	return m_songs_map.at(id);
}

std::unordered_map<SongId, SongItem> SongItems::getSongItems() const {
	return m_songs_map;
}

SongId SongItems::assign_id_internal() const {
	// use the last one with highest id
	auto it = std::max_element(m_songs_map.begin(), m_songs_map.end());
	if (it != m_songs_map.end()) return it->second.id+1;
	return 0; // empty map
}
