#include "songitems.hh"

#include "unicode.hh"
#include "libxml++-impl.hh"
#include <memory>
#include <string>


void SongItems::load(xmlpp::NodeSet const& n) {
	for (auto const& elem: n) {
		xmlpp::Element& element = dynamic_cast<xmlpp::Element&>(*elem);

		xmlpp::Attribute* a_id = element.get_attribute("id");
		if (!a_id) throw SongItemsException("No attribute id");

		xmlpp::Attribute* a_artist = element.get_attribute("artist");
		if (!a_artist) throw SongItemsException("No attribute artist");

		xmlpp::Attribute* a_title = element.get_attribute("title");
		if (!a_title) throw SongItemsException("No attribute title");

		addSongItem(a_artist->get_value(), a_title->get_value(), std::stoi(a_id->get_value()));
	}
}

void SongItems::save(xmlpp::Element* songs) {
	for (auto const& song: m_songs) {
		xmlpp::Element* element = xmlpp::add_child_element(songs, "song");
		element->set_attribute("id", std::to_string(song.id));
		element->set_attribute("artist", song.artist);
		element->set_attribute("title", song.title);
	}
}

SongId SongItems::addSongItem(std::string const& artist, std::string const& title, std::optional<SongId> _id) {
	SongItem si;
	si.id = _id.value_or(assign_id_internal());
	songMetadata collateInfo {{"artist", artist}, {"title", title}};
	UnicodeUtil::collate(collateInfo);		
	si.artist = collateInfo["artist"];
	si.title = collateInfo["title"];
	std::pair<songs_t::iterator, bool> ret = m_songs.insert(si);
	if (!ret.second)
	{
		si.id = assign_id_internal();
		m_songs.insert(si); // now do the insert with the fresh id
	}
	return si.id;
}

void SongItems::addSong(std::shared_ptr<Song> song) {
	SongItem si;
	si.id = lookup(song).value_or(addSongItem(song->artist, song->title));;
	auto it = m_songs.find(si);
	if (it == m_songs.end()) throw SongItemsException("Cant find song which was added just before");
	// it->song.reset(song); // does not work, it is a read only structure...

	// fill up the rest of the information
	si.artist = it->artist;
	si.title = it->title;
	si.song = song;

	m_songs.erase(it);
	m_songs.insert(si);
}

std::optional<SongId> SongItems::lookup(Song const& song) const {
	auto const& si = std::find_if(m_songs.begin(), m_songs.end(), [song](SongItem const& si) {
		return UnicodeUtil::toLower(song.collateByArtistOnly) == UnicodeUtil::toLower(si.artist) && UnicodeUtil::toLower(song.collateByTitleOnly) == UnicodeUtil::toLower(si.title);
	});
	if (si != m_songs.end()) return si->id;
	return std::nullopt;
}

std::optional<std::string> SongItems::lookup(const SongId &id) const {
	SongItem si;
	si.id = id;
	auto it = m_songs.find(si);
	if (it == m_songs.end()) return std::nullopt;
	else if (!it->song) return it->artist + " - " + it->title;
	else return it->song->artist + " - " + it->song->title;
}

SongId SongItems::assign_id_internal() const {
	// use the last one with highest id
	auto it = m_songs.rbegin();
	if (it != m_songs.rend()) return it->id+1;
	return 0; // empty set
}
