#include "file_time_song_order.hh"
#include "log.hh"

#include <filesystem>

namespace {
	std::chrono::seconds getFileWriteTime(Song const& song) {
		auto const time = std::filesystem::last_write_time(song.path);
		auto const seconds = std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch());
		SpdLogger::debug(LogSystem::SONGS, "Song file={}, time since last write={}", song.path, seconds.count());

		return seconds;
	}
}

std::string FileTimeSongOrder::getDescription() const {
	return _("sort by file time");
}

void FileTimeSongOrder::prepare(SongCollection const& songs, Database const&) {
	auto begin = songs.begin();
	auto end = songs.end();

	std::for_each(begin, end, [&](SongPtr const& song) {
		m_dateMap[song.get()] = getFileWriteTime(*song);
	});
}

bool FileTimeSongOrder::operator()(const Song& a, const Song& b) const {
	auto const dateA = m_dateMap.find(&a)->second;
	auto const dateB = m_dateMap.find(&b)->second;

	return dateA > dateB;
}

