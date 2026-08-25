#include "common.hh"

#include "game/song.hh"
#include "game/songparser-ini.hh"
#include "game/songparser-sm.hh"
#include "game/songparser-txt.hh"
#include "game/songparser-xml.hh"
#include "game/songparserfactory.hh"

#include <fstream>

namespace {
    // Real UltraStar TXT content (trimmed), from a representative karaoke song file.
    const std::string TXT_CONTENT =
        "#VERSION:1.2.0\n"
        "#TITLE:Holiday\n"
        "#ARTIST:Weezer\n"
        "#LANGUAGE:English\n"
        "#MP3:Weezer - Holiday.m4a\n"
        "#BPM:240\n"
        "#GAP:14150\n"
        ": 0 2 10 Let's \n"
        ": 3 3 15 go \n"
        "- 37\n"
        "E\n";

    // Real Frets on Fire song.ini content (trimmed), from a Rock Band-style song folder.
    const std::string INI_CONTENT =
        "[song]\n"
        "name = Pinball Wizard\n"
        "artist = The Who\n"
        "album = Tommy\n"
        "genre = Classic Rock\n"
        "year = 1969\n"
        "video = video-example.webm\n"
        "video_start_time = -3700\n"
        "preview_start_time = 16000\n";

    // Real StepMania SM content (trimmed), from an actual .sm chart file.
    const std::string SM_CONTENT =
        "#TITLE:Charlie bit me - auto tuned;\n"
        "#ARTIST:Unknown artist;\n"
        "#MUSIC:Charlie bit me Auto-Tuned.mp3;\n"
        "#OFFSET:-0.152;\n"
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
        ";\n";

    // Minimal SingStar XML content: just enough to satisfy xmlCheck's "<?" sniff.
    const std::string XML_CONTENT = "<?xml version=\"1.0\"?><MELODY></MELODY>";

    fs::path writeTempSongFile(std::string const& filename, std::string const& content) {
        auto dir = fs::temp_directory_path() / "performous_songparser_test";
        fs::create_directories(dir);
        auto path = dir / filename;
        std::ofstream(path, std::ios::binary) << content;
        return path;
    }

    Song makeSong(std::string const& filename, std::string const& content) {
        auto path = writeTempSongFile(filename, content);
        Song song;
        song.path = path.parent_path();
        song.filename = path;
        return song;
    }
}

TEST(UnitTest_SongParserFactory, detects_txt) {
    Song song = makeSong("holiday.txt", TXT_CONTENT);
    auto parser = SongParserFactory().create(song);

    EXPECT_EQ(Song::Type::TXT, song.type);
    EXPECT_NE(nullptr, dynamic_cast<TxtSongParser*>(parser.get()));
}

TEST(UnitTest_SongParserFactory, detects_ini) {
    Song song = makeSong("song.ini", INI_CONTENT);
    auto parser = SongParserFactory().create(song);

    EXPECT_EQ(Song::Type::INI, song.type);
    EXPECT_NE(nullptr, dynamic_cast<IniSongParser*>(parser.get()));
}

// iniCheck() matches [song] against the whole multi-line file buffer, so it can't lean on regex
// multiline mode (^/$ matching at line boundaries) to find it — MSVC's std::regex accepts the
// multiline flag but doesn't actually implement it, so a regex relying on it would only ever find
// [song] if it were literally the first and last thing in the file. Put [song] after a leading
// comment line, with real content both before and after it, to pin that down on every platform.
TEST(UnitTest_SongParserFactory, detects_ini_with_song_header_not_at_start_or_end_of_file) {
    Song song = makeSong("song.ini", "; exported by some editor\n" + INI_CONTENT);
    auto parser = SongParserFactory().create(song);

    EXPECT_EQ(Song::Type::INI, song.type);
    EXPECT_NE(nullptr, dynamic_cast<IniSongParser*>(parser.get()));
}

TEST(UnitTest_SongParserFactory, detects_sm) {
    Song song = makeSong("test.sm", SM_CONTENT);
    auto parser = SongParserFactory().create(song);

    EXPECT_EQ(Song::Type::SM, song.type);
    EXPECT_NE(nullptr, dynamic_cast<SmSongParser*>(parser.get()));
}

TEST(UnitTest_SongParserFactory, detects_xml) {
    Song song = makeSong("notes.xml", XML_CONTENT);
    auto parser = SongParserFactory().create(song);

    EXPECT_EQ(Song::Type::XML, song.type);
    EXPECT_NE(nullptr, dynamic_cast<XmlSongParser*>(parser.get()));
}

// SM songs look extremely similar to TXT songs structurally; the factory must sniff SM first
// or every SM song in the wild would silently get mis-parsed as TXT.
TEST(UnitTest_SongParserFactory, prefers_sm_over_txt_for_ambiguous_content) {
    Song song = makeSong("ambiguous.sm", SM_CONTENT);
    auto parser = SongParserFactory().create(song);

    EXPECT_EQ(Song::Type::SM, song.type);
}

TEST(UnitTest_SongParserFactory, rejects_content_too_small) {
    Song song = makeSong("tiny.txt", "#T:x\n");
    EXPECT_THROW(SongParserFactory().create(song), SongParserException);
}

TEST(UnitTest_SongParserFactory, rejects_unrecognized_content) {
    Song song = makeSong("garbage.txt", "This is just some plain prose, not a song file at all.\n");
    EXPECT_THROW(SongParserFactory().create(song), SongParserException);
}

TEST(UnitTest_SongParserFactory, rejects_missing_file) {
    Song song;
    song.path = fs::temp_directory_path();
    song.filename = song.path / "does-not-exist.txt";
    EXPECT_THROW(SongParserFactory().create(song), SongParserException);
}

// End-to-end: the factory-selected parser must actually extract real header fields correctly,
// not just pick the right format.
TEST(UnitTest_SongParserFactory, parses_real_ini_header_fields) {
    Song song = makeSong("song.ini", INI_CONTENT);
    auto parser = SongParserFactory().create(song);
    parser->parse(song);

    EXPECT_EQ("Pinball Wizard", song.title);
    EXPECT_EQ("The Who", song.artist);
    EXPECT_EQ("Classic Rock", song.genre);
    EXPECT_EQ("video-example.webm", song.video.filename().string());
}
