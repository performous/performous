#include "playlist.hh"
#include "song.hh"

void PlayList::addSong(boost::shared_ptr<Song> song)
{
	m_list.push_back(song);
}

bool PlayList::isEmpty()
{
	return m_list.empty();
}

boost::shared_ptr<Song> PlayList::getNext()
{
	if (isEmpty()) return boost::shared_ptr<Song>();
	boost::shared_ptr<Song> nextSong;
	nextSong = m_list[0];
	m_list.erase(m_list.begin());
	return nextSong;
}
