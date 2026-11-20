#include "common.hh"

#include "game/midifile.hh"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
	using Bytes = std::vector<std::uint8_t>;

	void append(Bytes& out, Bytes const& in) { out.insert(out.end(), in.begin(), in.end()); }
	void append(Bytes& out, std::string const& in) { out.insert(out.end(), in.begin(), in.end()); }
	void appendU32(Bytes& out, std::uint32_t v) { append(out, Bytes{std::uint8_t(v >> 24), std::uint8_t(v >> 16), std::uint8_t(v >> 8), std::uint8_t(v)}); }

	Bytes header(std::uint16_t format, std::uint16_t ntracks, std::uint16_t division) {
		Bytes b;
		append(b, "MThd");
		appendU32(b, 6);
		append(b, Bytes{std::uint8_t(format >> 8), std::uint8_t(format), std::uint8_t(ntracks >> 8), std::uint8_t(ntracks), std::uint8_t(division >> 8), std::uint8_t(division)});
		return b;
	}

	Bytes track(Bytes const& events) {
		Bytes b;
		append(b, "MTrk");
		appendU32(b, static_cast<std::uint32_t>(events.size()));
		append(b, events);
		return b;
	}

	Bytes metaEvent(std::uint8_t type, std::string const& data) {
		Bytes b{0x00, 0xFF, type, static_cast<std::uint8_t>(data.size())};
		append(b, data);
		return b;
	}

	const Bytes tempo120 = {0x00, 0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20};
	const Bytes endOfTrack = {0x00, 0xFF, 0x2F, 0x00};

	/// Writes the bytes to a unique temporary .mid file, removed on destruction
	struct TmpMidi {
		std::filesystem::path path;
		explicit TmpMidi(Bytes const& bytes) {
			static std::atomic<unsigned> counter{0};
			path = std::filesystem::temp_directory_path() / ("performous_midi_test_" + std::to_string(counter++) + ".mid");
			std::ofstream(path, std::ios::binary).write(reinterpret_cast<char const*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
		}
		~TmpMidi() { std::error_code ec; std::filesystem::remove(path, ec); }
	};
}

TEST(MidiFileParser, ParsesMinimalFile) {
	Bytes file = header(0, 1, 480);
	Bytes events = tempo120;
	append(events, endOfTrack);
	append(file, track(events));
	TmpMidi tmp(file);
	MidiFileParser midi(tmp.path);
	ASSERT_EQ(midi.tracks.size(), 1u);
	ASSERT_EQ(midi.tempochanges.size(), 1u);
	EXPECT_EQ(midi.tempochanges.front().value, 500000u);
}

TEST(MidiFileParser, LyricAfterRemovedVocalNoteDoesNotTouchEmptyLyrics) {
	// A note-off without lyric removes the only lyric note; a lyric event followed by another
	// note-off then used to write through lyrics.back() of an empty vector (heap corruption).
	Bytes file = header(0, 1, 480);
	Bytes events = tempo120;
	append(events, metaEvent(0x03, "PART VOCALS"));
	append(events, Bytes{0x00, 0x90, 0x3C, 0x40});  // note-on 60
	append(events, Bytes{0x10, 0x80, 0x3C, 0x00});  // note-off 60, no lyric -> lyric note removed
	append(events, metaEvent(0x05, "la"));           // lyric with no pending note
	append(events, Bytes{0x00, 0x80, 0x3C, 0x00});  // note-off again
	append(events, endOfTrack);
	append(file, track(events));
	TmpMidi tmp(file);
	MidiFileParser midi(tmp.path);
	ASSERT_EQ(midi.tracks.size(), 1u);
	EXPECT_TRUE(midi.tracks.front().lyrics.empty());
}
