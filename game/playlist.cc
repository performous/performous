#include "playlist.hh"
#include "song.hh"
#include <algorithm>
#include <random>

void PlayList::addSong(std::shared_ptr<Song> song) {
	std::lock_guard<std::mutex> l(m_mutex);
	m_list.push_back(song);
}

std::shared_ptr<Song> PlayList::getNext() {
	std::lock_guard<std::mutex> l(m_mutex);
	if (isEmpty()) return std::shared_ptr<Song>();
	std::shared_ptr<Song> nextSong;
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
	std::lock_guard<std::mutex> l(m_mutex);
	std::shuffle(m_list.begin(), m_list.end(), std::mt19937(std::random_device()()));
}

void PlayList::clear() {
	std::lock_guard<std::mutex> l(m_mutex);
	m_list.clear();
}

void PlayList::removeSong(int index) {
	std::lock_guard<std::mutex> l(m_mutex);
	m_list.erase(m_list.begin() + index);
}
void PlayList::swap(int index1, int index2) {
	std::lock_guard<std::mutex> l(m_mutex);
	std::shared_ptr<Song> song1 = m_list[index1];
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


std::shared_ptr<Song> PlayList::getSong(int index) {
	std::lock_guard<std::mutex> l(m_mutex);
	if (isEmpty()) return std::shared_ptr<Song>();
	std::shared_ptr<Song> nextSong;
	nextSong = m_list[index];
	m_list.erase(m_list.begin() + index);
	currentlyActive = nextSong;
	return nextSong;
}
