// Split out of song.cc: these methods need ffmpeg (duration probing) and ScreenSing (playback status),
// which would otherwise drag the audio/graphics stack into anything that links Song (see song.hh).
#include "song.hh"

#include "ffmpeg.hh"
#include "log.hh"
#include "screen_sing.hh"
#include "songparserutil.hh"

#include <algorithm>
#include <cmath>

Song::Status Song::status(double time, ScreenSing* song) {
	if (song->getMenu().isOpen()) return Status::NORMAL; // This should prevent querying getVocalTrack with an out-of-bounds/uninitialized index.
	if (vocalTracks.empty()) return Status::NORMAL;	 // To avoid crash with non-vocal songs (dance, guitar) -- FIXME: what should we actually do?
	Note target; target.end = time;
	Notes* notes = nullptr;
	Notes::const_iterator it;

	if (song->singingDuet()) {
		notes = &getVocalTrack(SongParserUtil::DUET_BOTH).notes;
	}
	else {
		notes = &getVocalTrack(song->selectedVocalTrack()).notes;
	}
	it = std::lower_bound(notes->begin(), notes->end(), target, [](Note const& a, Note const& b) { return a.end < b.end; });
	if (it == notes->end()) return Status::FINISHED;
	if (it->begin > time + 4.0) return Status::INSTRUMENTAL_BREAK;
	return Status::NORMAL;
}

double Song::getDurationSeconds() {
	if (m_duration == 0.0 || m_duration < 1.0) {
		try {
			auto ffmpeg = std::make_unique<DurationFFmpeg>(music[TrackName::BGMUSIC]);
			m_duration = ffmpeg->duration();
			return m_duration;
		}
		catch (FFmpeg::Error const& e) {
			SpdLogger::warn(LogSystem::SONGS, "Couldn't open file for calculating duration. FFMPEG error={}", e.what());
			return 0.0;
		}
	}
	else { //duration is still in memmory that means we already loaded it
		return m_duration;
	}
}

double Song::getPreviewStart() {
	if (std::isnan(preview_start)) {
		preview_start = ((type == Type::INI || getDurationSeconds() < 50.0) ? 5.0 : 30.0);	// 5 s for band mode, 30 s for others
	}
	return preview_start;
}
