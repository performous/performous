#include "common.hh"

#include "game/song.hh"
#include "game/songparser-txt.hh"
#include "game/songparserutil.hh"

#include <cmath>
#include <fstream>

namespace {
    // Real UltraStar TXT content (trimmed), from a representative karaoke song file:
    // BPM 240, GAP 14150ms, so the first note should begin at ~14.15s.
    const std::string TXT_CONTENT =
        "#TITLE:Holiday\n"
        "#ARTIST:Weezer\n"
        "#MP3:Weezer - Holiday.m4a\n"
        "#BPM:240\n"
        "#GAP:14150\n"
        ": 0 2 10 Let's \n"
        ": 3 3 15 go \n"
        "- 37\n"
        ": 40 2 14 You \n"
        "E\n";

    // A compact duet excerpt, structurally modeled on a real multi-singer karaoke file
    // (DUETSINGERP1/P2 tags plus "P 1"/"P 2" blocks).
    const std::string DUET_TXT_CONTENT =
        "#TITLE:Prisoner\n"
        "#ARTIST:Miley Cyrus and Dua Lipa\n"
        "#MP3:Prisoner.mp3\n"
        "#BPM:253.37\n"
        "#GAP:4257.0\n"
        "#DUETSINGERP1:Miley Cyrus\n"
        "#DUETSINGERP2:Dua Lipa\n"
        "P 1\n"
        ": 0 2 5 Pris\n"
        ": 4 2 5 on\n"
        ": 7 5 3 er\n"
        "- 20\n"
        "P 2\n"
        ": 20 2 6 Strung\n"
        ": 24 3 6  out\n"
        "- 40\n"
        "E\n";

    // Makes a Song whose referenced BGMUSIC file actually exists on disk, since TxtSongParser
    // flags PARSERERROR when it doesn't (see missing_audio_file_flags_parser_error below).
    Song makeSongWithAudio(std::string const& mp3Name) {
        auto dir = fs::temp_directory_path() / "performous_txtsongparser_test";
        fs::create_directories(dir);
        std::ofstream(dir / mp3Name, std::ios::binary) << "fake audio data";

        Song song;
        song.path = dir;
        song.filename = dir / "song.txt";
        return song;
    }
}

// This is the scenario the SongParserFactory redesign has to get right: the header pass and the
// notes pass each get a fresh TxtSongParser instance (mirroring what the factory does on reload),
// so the second pass can't rely on any state left over in the first pass's parser object.
TEST(UnitTest_TxtSongParser, two_pass_load_recovers_gap_after_header_only_pass) {
    Song song = makeSongWithAudio("Weezer - Holiday.m4a");

    TxtSongParser(TXT_CONTENT).parse(song);
    ASSERT_EQ(Song::LoadStatus::HEADER, song.loadStatus);
    EXPECT_EQ("Holiday", song.title);
    EXPECT_EQ("Weezer", song.artist);

    // Simulate the factory creating a brand new parser instance for the notes pass.
    TxtSongParser(TXT_CONTENT).parse(song);
    ASSERT_EQ(Song::LoadStatus::FULL, song.loadStatus);

    auto const& notes = song.getVocalTrack(TrackName::VOCAL_LEAD).notes;
    ASSERT_FALSE(notes.empty());
    // GAP:14150 means the first note should begin at ~14.15s, not 0s (which is what it would be if
    // the recovered gap were lost between the two passes).
    EXPECT_NEAR(14.15, notes.front().begin, 0.01);
}

TEST(UnitTest_TxtSongParser, missing_audio_file_flags_parser_error) {
    Song song;
    song.path = fs::temp_directory_path();
    song.filename = song.path / "song.txt"; // music file below is never created on disk

    TxtSongParser(TXT_CONTENT).parse(song);

    EXPECT_EQ(Song::LoadStatus::PARSERERROR, song.loadStatus);
}

TEST(UnitTest_TxtSongParser, duet_singers_are_merged_into_together_track) {
    Song song = makeSongWithAudio("Prisoner.mp3");

    TxtSongParser(DUET_TXT_CONTENT).parse(song);
    TxtSongParser(DUET_TXT_CONTENT).parse(song);
    ASSERT_EQ(Song::LoadStatus::FULL, song.loadStatus);

    ASSERT_TRUE(song.hasDuet());
    auto const& duet = song.getVocalTrack(SongParserUtil::DUET_BOTH);
    // Notes from both P1 and P2 blocks should have been merged into the combined track.
    EXPECT_GE(duet.notes.size(), 4u);
}
