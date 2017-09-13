#include "songitems.hh"

#include "unicode.hh"

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <libxml++/libxml++.h>


void SongItems::load(xmlpp::NodeSet const& n) {
	for (auto const& elem: n) {
		xmlpp::Element& element = dynamic_cast<xmlpp::Element&>(*elem);

		xmlpp::Attribute* a_id = element.get_attribute("id");
		if (!a_id) throw SongItemsException("No attribute id");

		xmlpp::Attribute* a_artist = element.get_attribute("artist");
		if (!a_artist) throw SongItemsException("No attribute artist");

		xmlpp::Attribute* a_title = element.get_attribute("title");
		if (!a_title) throw SongItemsException("No attribute title");

		addSongItem(a_artist->get_value(), a_title->get_value(), boost::lexical_cast<int>(a_id->get_value()));
	}
}

void SongItems::save(xmlpp::Element* songs) {
	for (auto const& song: m_songs) {
		xmlpp::Element* element = songs->add_child("song");
		element->set_attribute("id", std::to_string(song.id));
		element->set_attribute("artist", song.artist);
		element->set_attribute("title", song.title);
	}
}

int SongItems::addSongItem(std::string const& artist, std::string const& title, int id) {
	SongItem si;
	if (id==-1) id = assign_id_internal();
	si.id = id;
	si.artist = unicodeCollate(artist);
	si.title = unicodeCollate(title);

	std::pair<songs_t::iterator, bool> ret = m_songs.insert(si);
	if (!ret.second)
	{
		si.id = assign_id_internal();
		m_songs.insert(si); // now do the insert with the fresh id
	}
	return si.id;
}

void SongItems::addSong(boost::shared_ptr<Song> song) {
	int id = lookup(song);
	if (id == -1)
	{
		id = addSongItem(song->artist, song->title);
	}

	SongItem si;
	si.id = id;
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

int SongItems::lookup(boost::shared_ptr<Song> song) const {
	for (auto const& s: m_songs) {
		if (song->collateByArtistOnly == s.artist && song->collateByTitleOnly == s.title) return s.id;
	}
	return -1;
}

int SongItems::lookup(Song& song) const {
	for (auto const& s: m_songs) {
		if (song.collateByArtistOnly == s.artist && song.collateByTitleOnly == s.title) return s.id;
	}
	return -1;
}

std::string SongItems::lookup(int id) const {
	SongItem si;
	si.id = id;
	auto it = m_songs.find(si);
	if (it == m_songs.end()) return "Unknown Song";
	else if (!it->song) return it->artist + " - " + it->title;
	else return it->song->artist + " - " + it->song->title;
}

int SongItems::assign_id_internal() const {
	// use the last one with highest id
	auto it = m_songs.rbegin();
	if (it != m_songs.rend()) return it->id+1;
	else return 1; // empty set
}
