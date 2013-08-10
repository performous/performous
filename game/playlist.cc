#include "playlist.hh"
#include "song.hh"
#include <algorithm>

void PlayList::addSong(boost::shared_ptr<Song> song) {
	boost::mutex::scoped_lock(m_mutex);
	m_list.push_back(song);
}

boost::shared_ptr<Song> PlayList::getNext() {
	boost::mutex::scoped_lock(m_mutex);
	if (isEmpty()) return boost::shared_ptr<Song>();
	boost::shared_ptr<Song> nextSong;
	nextSong = m_list[0];
	m_list.erase(m_list.begin());
	currentlyActive == nextSong;
	return nextSong;
}

PlayList::SongList& PlayList::getList() {
	return m_list;
}


bool PlayList::isEmpty() {
	return m_list.empty();
}

void PlayList::shuffle() {
	boost::mutex::scoped_lock(m_mutex);
	std::random_shuffle(m_list.begin(), m_list.end());
}

void PlayList::clear() {
	boost::mutex::scoped_lock(m_mutex);
	m_list.clear();
}

void PlayList::removeSong(int index) {
	boost::mutex::scoped_lock(m_mutex);
	m_list.erase(m_list.begin() + index);
}

boost::shared_ptr<Song> PlayList::getSong(int index) {
	boost::mutex::scoped_lock(m_mutex);
	if (isEmpty()) return boost::shared_ptr<Song>();
	boost::shared_ptr<Song> nextSong;
	nextSong = m_list[index];
	m_list.erase(m_list.begin() + index);
	currentlyActive == nextSong;
	return nextSong;
}
