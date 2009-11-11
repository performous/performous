#include "songitems.hh"

#include "unicode.hh"

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <libxml++/libxml++.h>


void SongItems::load(xmlpp::NodeSet const& n) {
	for (xmlpp::NodeSet::const_iterator it = n.begin(); it != n.end(); ++it)
	{
		xmlpp::Element& element = dynamic_cast<xmlpp::Element&>(**it);

		xmlpp::Attribute* a_id = element.get_attribute("id");
		if (!a_id) throw SongItemsException("No attribute id");

		xmlpp::Attribute* a_artist = element.get_attribute("artist");
		if (!a_artist) throw SongItemsException("No attribute artist");

		xmlpp::Attribute* a_title = element.get_attribute("title");
		if (!a_title) throw SongItemsException("No attribute title");

		addSongItem(a_artist->get_value(), a_title->get_value(), boost::lexical_cast<int>(a_id->get_value()));
	}
}

void SongItems::save(xmlpp::Element *songs) {
	for (songs_t::const_iterator it = m_songs.begin(); it != m_songs.end(); ++it)
	{
		xmlpp::Element* song = songs->add_child("song");
		song->set_attribute("id", boost::lexical_cast<std::string>(it->id));
		song->set_attribute("artist", it->artist);
		song->set_attribute("title", it->title);
	}
}

void SongItems::addSongItem(std::string const& artist, std::string const& title, int id) {
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
}

void SongItems::addSong(boost::shared_ptr<Song> song) {
	if (lookup(song) == -1) addSongItem(song->artist, song->title);
}

int SongItems::lookup(boost::shared_ptr<Song> song) {
	for (songs_t::iterator it = m_songs.begin(); it != m_songs.end(); ++it)
	{
		if (song->collateByArtistOnly == it->artist && song->collateByTitleOnly == it->title) return it->id;
	}
	return -1;
}

int SongItems::assign_id_internal() {
	songs_t::const_reverse_iterator it = m_songs.rbegin();
	if (it != m_songs.rend()) return it->id+1;
	else return 1; // empty set
}
