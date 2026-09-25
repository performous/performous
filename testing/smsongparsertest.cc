#include "common.hh"

#include "game/song.hh"
#include "game/songparser-sm.hh"

#include <algorithm>

namespace {
    // Real StepMania SM content (trimmed to 2 measures), single BPM, non-zero OFFSET:
    // OFFSET:-0.152 means the song's gap is +0.152s (SM negates OFFSET into the internal gap).
    const std::string SM_CONTENT_SINGLE_BPM =
        "#TITLE:Charlie bit me - auto tuned;\n"
        "#ARTIST:Unknown artist;\n"
        "#MUSIC:Charlie bit me Auto-Tuned.mp3;\n"
        "#OFFSET:-0.152;\n"
        "#SAMPLESTART:9.960;\n"
        "#SAMPLELENGTH:12.000;\n"
        "#BPMS:0.000=152.000;\n"
        "#STOPS:;\n"
        "#NOTES:\n"
        "     dance-single:\n"
        "     :\n"
        "     Challenge:\n"
        "     7:\n"
        "     0.899,0.844,0.720,0.360,0.288:  // measure 1\n"
        "0010\n"
        "0100\n"
        "1000\n"
        "0100\n"
        "0001\n"
        "0010\n"
        "0100\n"
        "1000\n"
        ",  // measure 2\n"
        "0010\n"
        "1000\n"
        "0100\n"
        "0001\n"
        "0100\n"
        "1000\n"
        "0100\n"
        "0010\n"
        ";\n";

    // Real StepMania SM content (trimmed to 2 measures) with multiple BPM changes and a
    // matched hold note (HOLDBEGIN '2' in measure 1, HOLDEND '3' in measure 2) — the kind of
    // file that stresses BPM-segment lookup and cross-measure hold tracking at once.
    const std::string SM_CONTENT_MULTI_BPM =
        "#TITLE:Aerodynamic;\n"
        "#ARTIST:Daft Punk;\n"
        "#MUSIC:Daft Punk - Aerodynamic.mp3;\n"
        "#OFFSET:0.000;\n"
        "#BPMS:0.000=122.930,128.000=122.880,304.000=122.700,352.000=123.100;\n"
        "#STOPS:;\n"
        "#NOTES:\n"
        "     dance-single:\n"
        "     :\n"
        "     Challenge:\n"
        "     13:\n"
        "     1.000,1.000,1.000,0.289,1.000:  // measure 1\n"
        "2002\n"
        "0000\n"
        "0000\n"
        "0000\n"
        ",  // measure 2\n"
        "0000\n"
        "0000\n"
        "0000\n"
        "0000\n"
        "0000\n"
        "0000\n"
        "0000\n"
        "0000\n"
        "0000\n"
        "0000\n"
        "0000\n"
        "0000\n"
        "0000\n"
        "0000\n"
        "0000\n"
        "3003\n"
        ";\n";

    // guessFiles() (run as part of every parse pass) needs a real directory to scan.
    Song makeSong() {
        Song song;
        song.path = fs::temp_directory_path();
        return song;
    }
}

// Mirrors what the factory does across a header pass and a notes pass: each gets a fresh
// SmSongParser instance, so nothing about the song must depend on parser state surviving between them.
TEST(UnitTest_SmSongParser, two_pass_load_parses_real_notes) {
    Song song = makeSong();

    SmSongParser(SM_CONTENT_SINGLE_BPM).parse(song);
    ASSERT_EQ(Song::LoadStatus::HEADER, song.loadStatus);
    EXPECT_EQ("Charlie bit me - auto tuned", song.title);
    EXPECT_EQ("Unknown artist", song.artist);

    SmSongParser(SM_CONTENT_SINGLE_BPM).parse(song);
    ASSERT_EQ(Song::LoadStatus::FULL, song.loadStatus);

    ASSERT_NE(song.danceTracks.end(), song.danceTracks.find("dance-single"));
    auto const& difficulties = song.danceTracks.at("dance-single");
    ASSERT_NE(difficulties.end(), difficulties.find(DanceDifficulty::CHALLENGE));
    auto const& notes = difficulties.at(DanceDifficulty::CHALLENGE).notes;

    // Two measures of real step data with a handful of taps each; just confirm real notes came through.
    EXPECT_GE(notes.size(), 10u);
    for (auto const& note : notes) {
        EXPECT_GE(note.begin, 0.0);
    }
    // Notes should be in non-decreasing time order across the two measures.
    EXPECT_TRUE(std::is_sorted(notes.begin(), notes.end(), [](Note const& a, Note const& b) { return a.begin < b.begin; }));
}

TEST(UnitTest_SmSongParser, multi_bpm_file_parses_without_error_and_orders_notes) {
    Song song = makeSong();

    SmSongParser(SM_CONTENT_MULTI_BPM).parse(song);
    SmSongParser(SM_CONTENT_MULTI_BPM).parse(song);
    ASSERT_EQ(Song::LoadStatus::FULL, song.loadStatus);

    auto const& notes = song.danceTracks.at("dance-single").at(DanceDifficulty::CHALLENGE).notes;
    ASSERT_FALSE(notes.empty());
    EXPECT_TRUE(std::is_sorted(notes.begin(), notes.end(), [](Note const& a, Note const& b) { return a.begin < b.begin; }));

    // The excerpt's hold note (HOLDBEGIN in measure 1, matching HOLDEND in measure 2) must have
    // been closed with an end time strictly after its begin time, not left at its default.
    auto hold = std::find_if(notes.begin(), notes.end(), [](Note const& n) { return n.type == Note::Type::HOLDBEGIN; });
    ASSERT_NE(notes.end(), hold);
    EXPECT_GT(hold->end, hold->begin);
}
