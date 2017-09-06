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
	currentlyActive = nextSong;
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
void PlayList::swap(int index1, int index2) {
	boost::mutex::scoped_lock(m_mutex);
	boost::shared_ptr<Song> song1 = m_list[index1];
	m_list[index1] = m_list[index2];
	m_list[index2] = song1;
}
void PlayList::setPosition(unsigned int index1, unsigned int index2) {
	if(index1 != index2) {	
		int diff = index1 - index2;
		if(diff > 0) {
			// Going to Top
			swap(index1, index1 - 1);
			setPosition(index1 - 1, index2);
		} else {
			// Going to Bottom
			swap(index1, index1 + 1);
			setPosition(index1 + 1, index2);
		} 
	}
}


boost::shared_ptr<Song> PlayList::getSong(int index) {
	boost::mutex::scoped_lock(m_mutex);
	if (isEmpty()) return boost::shared_ptr<Song>();
	boost::shared_ptr<Song> nextSong;
	nextSong = m_list[index];
	m_list.erase(m_list.begin() + index);
	currentlyActive = nextSong;
	return nextSong;
}
