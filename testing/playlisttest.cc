#include "common.hh"

#include "game/playlist.hh"

namespace {
	void fill(PlayList& list, unsigned count) {  // PlayList holds a mutex, so it can't be returned by value
		for (unsigned i = 0; i < count; ++i) list.addSong(std::make_shared<Song>());
	}
}

TEST(PlayList, OutOfRangeIndicesAreIgnored) {
	// Indices come from web clients (/api/remove, /api/move): they used to erase/rotate out of bounds.
	PlayList list;
	fill(list, 2);
	EXPECT_FALSE(list.removeSong(4000000000u));
	EXPECT_FALSE(list.removeSong(2));
	list.swap(0, 7);
	list.move(9, 0);
	list.move(0, 9);
	EXPECT_EQ(list.getSong(2), nullptr);
	EXPECT_EQ(list.getList().size(), 2u);
}

TEST(PlayList, ValidIndicesStillWork) {
	PlayList list;
	fill(list, 3);
	auto first = list.getList()[0];
	auto last = list.getList()[2];
	list.swap(0, 2);
	EXPECT_EQ(list.getList()[0], last);
	EXPECT_EQ(list.getList()[2], first);
	EXPECT_TRUE(list.removeSong(1));
	EXPECT_EQ(list.getList().size(), 2u);
	EXPECT_EQ(list.getSong(0), last);
	EXPECT_EQ(list.getList().size(), 1u);
}
