#include "file_time_song_order.hh"

#include <filesystem>

namespace {
	std::chrono::seconds getFileWriteTime(Song const& song) {
		auto const time = std::filesystem::last_write_time(song.path);
		auto const seconds = std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch());

		std::cout << song.path << ": " << seconds.count() << std::endl;

		return seconds;
	}
}

std::string FileTimeSongOrder::getDescription() const {
	return _("sort by file time");
}

void FileTimeSongOrder::initialize(SongCollection const& songs, Database const&) {
	if (initialized)
		return;

	auto begin = songs.begin();
	auto end = songs.end();

	std::for_each(begin, end, [&](SongPtr const& song) {
		m_dateMap[song.get()] = getFileWriteTime(*song);
	});

	initialized = true;
}

bool FileTimeSongOrder::operator()(const Song& a, const Song& b) const {
	if (!initialized)
		return false;

	auto const dateA = m_dateMap.find(&a)->second;
	auto const dateB = m_dateMap.find(&b)->second;

	return dateA > dateB;
}

