#include "songitems.hh"

#include "unicode.hh"
#include "libxml++-impl.hh"

#include <algorithm>
#include <memory>
#include <string>


void SongItems::load(xmlpp::NodeSet const& n) {
    for (auto const& elem : n) {
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

        auto a_broken = element.get_attribute("broken");
        auto const broken = (a_broken && a_broken->get_value() == "true");

		xmlpp::Attribute* a_timesPlayed = element.get_attribute("timesPlayed");
		auto timesPlayed = -1;
		if (a_timesPlayed)
			timesPlayed = std::stoi(a_timesPlayed->get_value());

		addSongItem(artist, title, broken, timesPlayed, id);
    }
}

void SongItems::save(xmlpp::Element* songs) {
	for (auto const& [song_id, song] : m_songs_map) {
        xmlpp::Element* element = xmlpp::add_child_element(songs, "song");
        element->set_attribute("id", std::to_string(song.id));
        element->set_attribute("artist", song.artist);
        element->set_attribute("title", song.title);
		element->set_attribute("broken", song.isBroken() ? "true" : "false");
		element->set_attribute("timesPlayed", std::to_string(song.timesPlayed));
    }
}

SongId SongItems::addSongItem(std::string const& artist, std::string const& title, bool broken, std::optional<int> const& _timesPlayed, std::optional<SongId> _id) {
    SongItem si;

	if (_id.has_value() && m_songs_map.find(_id.value()) != m_songs_map.end()) {
		si = m_songs_map.at(_id.value());
	}

    si.id = _id.value_or(assign_id_internal());
    songMetadata collateInfo{ {"artist", artist}, {"title", title} };
    UnicodeUtil::collate(collateInfo);
    si.artist = collateInfo["artist"];
    si.title = collateInfo["title"];
    si.setBroken(broken);
	si.timesPlayed = _timesPlayed.value_or(si.timesPlayed);

	m_songs_map[si.id] = si;
    return si.id;
}

SongId SongItems::addSong(SongPtr song) {
	// if song is already in db verify integrity and return
	if (song->id >= 0 && m_songs_map.find(song->id) != m_songs_map.end()) {
		// verify artist and title match
		if (matchArtistAndTitle(*song, m_songs_map.at(song->id)))
			return song->id;
		// else the song has a wrong ID and should take on whichever ID is returned below
	}

	auto const& song_id = resolveToSongId(*song).value_or(addSongItem(song->artist, song->title));

    SongItem si = m_songs_map.at(song_id);

	// if a song was not in the cache, but had hiscores in the db which were counted to set an initial value for timesPlayed make sure to use that instead of 0
    si.timesPlayed = si.timesPlayed < 0 ? song->timesPlayed : std::max(song->timesPlayed, uint32_t(si.timesPlayed));
    si.setBroken(it->isBroken());
    si.setSong(song);
	song->id = song_id;

	m_songs_map[si.id] = si;

	return si.id;
}

std::optional<SongId> SongItems::resolveToSongId(Song const& song) const {
	auto const si = std::find_if(m_songs_map.begin(), m_songs_map.end(), [&song, this](std::pair<const SongId, SongItem> const& si) {
		return match_artist_and_title_internal(song, si.second);
	});
	if (si != m_songs_map.end())
		return si->second.id;
	return std::nullopt;
}

bool SongItems::match_artist_and_title_internal(Song const& song, SongItem const& songItem) const {
        // This is not always really correct but in most cases these inputs should have been normalized into unicode at one point during their life time.
	if (song.collateByArtistOnly.length() != songItem.artist.length() || song.collateByTitleOnly.length() != songItem.title.length())
		return false;

	return UnicodeUtil::caseEqual(song.collateByArtistOnly, songItem.artist, true) && UnicodeUtil::caseEqual(song.collateByTitleOnly, songItem.title, true);
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
    if (it != m_songs_map.end())
        return it->second.id + 1;
    return 0; // empty map
}


std::shared_ptr<Song> SongItem::getSong() const {
    return m_song;
}

void SongItem::setSong(std::shared_ptr<Song> song) {
    if (song)
        song->setBroken(m_broken);

    m_song = song;
}

bool SongItem::isBroken() const {
    if (m_song)
        return m_song->isBroken();

    return m_broken;
}

void SongItem::setBroken(bool broken) {
    if (m_song)
        m_song->setBroken(broken);

    m_broken = broken;
}
